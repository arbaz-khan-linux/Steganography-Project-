#include <stdio.h>
#include <string.h>
#include "encode.h"
#include "types.h"
#include "common.h"

/* Function Definitions */

Status read_and_validate_encode_args(char *argv[], EncodeInfo *encInfo)
{
    if (argv[2] == NULL || strstr(argv[2], ".bmp") == NULL)
    {
        return e_failure;
    }
    encInfo->src_image_fname = argv[2];

    if (argv[3] == NULL || strstr(argv[3], ".") == NULL)
    {
        return e_failure;
    }
    encInfo->secret_fname = argv[3];
    strcpy(encInfo->extn_secret_file, strstr(argv[3], "."));

    if (argv[4] == NULL)
    {
        encInfo->stego_image_fname = "stego.bmp";
    }
    else
    {
        encInfo->stego_image_fname = argv[4];
    }

    return e_success;
}

/* Get image size
 * Input: Image file ptr
 * Output: width * height * bytes per pixel (3 in our case)
 * Description: In BMP Image, width is stored in offset 18,
 * and height after that. size is 4 bytes
 */
uint get_image_size_for_bmp(FILE *fptr_image)
{
    uint width, height;

    fseek(fptr_image, 18, SEEK_SET);

    fread(&width, sizeof(int), 1, fptr_image);
    printf("width = %u\n", width);

    fread(&height, sizeof(int), 1, fptr_image);
    printf("height = %u\n", height);

    return width * height * 3;
}

/*
 * Get File pointers for i/p and o/p files
 * Inputs: Src Image file, Secret file and
 * Stego Image file
 * Output: FILE pointer for above files
 * Return Value: e_success or e_failure, on file errors
 */
Status open_files(EncodeInfo *encInfo)
{
    encInfo->fptr_src_image = fopen(encInfo->src_image_fname, "r");
    if (encInfo->fptr_src_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->src_image_fname);

        return e_failure;
    }

    encInfo->fptr_secret = fopen(encInfo->secret_fname, "r");
    if (encInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->secret_fname);

        return e_failure;
    }

    encInfo->fptr_stego_image = fopen(encInfo->stego_image_fname, "w");
    if (encInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", encInfo->stego_image_fname);

        return e_failure;
    }

    return e_success;
}

uint get_file_size(FILE *fptr)
{
    fseek(fptr, 0, SEEK_END);

    return ftell(fptr);
}

Status check_capacity(EncodeInfo *encInfo)
{
    encInfo->image_capacity = get_image_size_for_bmp(encInfo->fptr_src_image);

    encInfo->size_secret_file = get_file_size(encInfo->fptr_secret);

    uint bits_needed = (strlen(MAGIC_STRING) + sizeof(int) + strlen(encInfo->extn_secret_file)
                        + sizeof(int) + encInfo->size_secret_file) * 8;

    if (encInfo->image_capacity > bits_needed)
    {
        return e_success;
    }

    return e_failure;
}

Status copy_bmp_header(FILE *fptr_src_image, FILE *fptr_dest_image)
{
    char header[54];

    rewind(fptr_src_image);

    if (fread(header, 54, 1, fptr_src_image) != 1)
    {
        return e_failure;
    }

    fwrite(header, 54, 1, fptr_dest_image);

    return e_success;
}

Status encode_byte_to_lsb(char data, char *image_buffer)
{
    for (int i = 0; i < 8; i++)
    {
        image_buffer[i] = (image_buffer[i] & 0xFE) | ((data >> (7 - i)) & 1);
    }

    return e_success;
}

Status encode_data_to_image(char *data, int size, FILE *fptr_src_image, FILE *fptr_stego_image)
{
    char arr[8];

    for (int i = 0; i < size; i++)
    {
        if (fread(arr, 8, 1, fptr_src_image) != 1)
        {
            return e_failure;
        }

        encode_byte_to_lsb(data[i], arr);

        fwrite(arr, 8, 1, fptr_stego_image);
    }

    return e_success;
}

Status encode_magic_string(const char *magic_string, EncodeInfo *encInfo)
{
    return encode_data_to_image((char *)magic_string, strlen(magic_string),
                                 encInfo->fptr_src_image, encInfo->fptr_stego_image);
}

Status encode_secret_file_extn_size(int size, EncodeInfo *encInfo)
{
    return encode_data_to_image((char *)&size, sizeof(int),
                                 encInfo->fptr_src_image, encInfo->fptr_stego_image);
}

Status encode_secret_file_extn(const char *file_extn, EncodeInfo *encInfo)
{
    return encode_data_to_image((char *)file_extn, strlen(file_extn),
                                 encInfo->fptr_src_image, encInfo->fptr_stego_image);
}

Status encode_secret_file_size(long file_size, EncodeInfo *encInfo)
{
    int size = (int)file_size;

    return encode_data_to_image((char *)&size, sizeof(int),
                                 encInfo->fptr_src_image, encInfo->fptr_stego_image);
}

Status encode_secret_file_data(EncodeInfo *encInfo)
{
    char ch;
    char arr[8];

    rewind(encInfo->fptr_secret);

    for (long i = 0; i < encInfo->size_secret_file; i++)
    {
        ch = fgetc(encInfo->fptr_secret);

        if (fread(arr, 8, 1, encInfo->fptr_src_image) != 1)
        {
            return e_failure;
        }

        encode_byte_to_lsb(ch, arr);

        fwrite(arr, 8, 1, encInfo->fptr_stego_image);
    }

    return e_success;
}

Status copy_remaining_img_data(FILE *fptr_src, FILE *fptr_dest)
{
    char ch;

    while (fread(&ch, 1, 1, fptr_src) == 1)
    {
        fwrite(&ch, 1, 1, fptr_dest);
    }

    return e_success;
}

Status do_encoding(EncodeInfo *encInfo)
{
    if (open_files(encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to open files\n");
        return e_failure;
    }
    printf("INFO: Opened all required files\n");

    if (check_capacity(encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Source image doesn't have enough capacity to encode the secret file\n");
        return e_failure;
    }
    printf("INFO: Capacity check passed\n");

    if (copy_bmp_header(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to copy BMP header\n");
        return e_failure;
    }
    printf("INFO: Copied BMP header\n");

    if (encode_magic_string(MAGIC_STRING, encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to encode magic string\n");
        return e_failure;
    }
    printf("INFO: Encoded magic string\n");

    if (encode_secret_file_extn_size(strlen(encInfo->extn_secret_file), encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to encode secret file extension size\n");
        return e_failure;
    }

    if (encode_secret_file_extn(encInfo->extn_secret_file, encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to encode secret file extension\n");
        return e_failure;
    }
    printf("INFO: Encoded secret file extension\n");

    if (encode_secret_file_size(encInfo->size_secret_file, encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to encode secret file size\n");
        return e_failure;
    }
    printf("INFO: Encoded secret file size\n");

    if (encode_secret_file_data(encInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to encode secret file data\n");
        return e_failure;
    }
    printf("INFO: Encoded secret file data\n");

    if (copy_remaining_img_data(encInfo->fptr_src_image, encInfo->fptr_stego_image) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to copy remaining image data\n");
        return e_failure;
    }
    printf("INFO: Copied remaining image data, encoding done successfully\n");

    fclose(encInfo->fptr_src_image);
    fclose(encInfo->fptr_secret);
    fclose(encInfo->fptr_stego_image);

    return e_success;
}
