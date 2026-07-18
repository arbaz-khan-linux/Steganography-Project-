#include <stdio.h>
#include <string.h>
#include "decode.h"
#include "types.h"
#include "common.h"

Status read_and_validate_decode_args(char *argv[], DecodeInfo *decInfo)
{
    if (argv[2] == NULL || strstr(argv[2], ".bmp") == NULL)
    {
        return e_failure;
    }
    decInfo->stego_image_fname = argv[2];

    if (argv[3] != NULL)
    {
        decInfo->secret_fname = argv[3];
    }
    else
    {
        decInfo->secret_fname = "decoded";
    }

    return e_success;
}

Status open_files_decode(DecodeInfo *decInfo)
{
    decInfo->fptr_stego_image = fopen(decInfo->stego_image_fname, "r");
    if (decInfo->fptr_stego_image == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->stego_image_fname);

        return e_failure;
    }

    return e_success;
}

Status decode_byte_from_lsb(char *data, char *image_buffer)
{
    *data = 0;

    for (int i = 0; i < 8; i++)
    {
        *data = (*data << 1) | (image_buffer[i] & 1);
    }

    return e_success;
}

Status decode_data_from_image(char *data, int size, FILE *fptr_stego_image)
{
    char arr[8];

    for (int i = 0; i < size; i++)
    {
        if (fread(arr, 8, 1, fptr_stego_image) != 1)
        {
            return e_failure;
        }

        decode_byte_from_lsb(&data[i], arr);
    }

    return e_success;
}

Status decode_magic_string(DecodeInfo *decInfo)
{
    char magic[3];

    fseek(decInfo->fptr_stego_image, 54, SEEK_SET);

    if (decode_data_from_image(magic, strlen(MAGIC_STRING), decInfo->fptr_stego_image) == e_failure)
    {
        return e_failure;
    }
    magic[strlen(MAGIC_STRING)] = '\0';

    if (strcmp(magic, MAGIC_STRING) == 0)
    {
        return e_success;
    }

    return e_failure;
}

Status decode_secret_file_extn_size(DecodeInfo *decInfo)
{
    int size;

    if (decode_data_from_image((char *)&size, sizeof(int), decInfo->fptr_stego_image) == e_failure)
    {
        return e_failure;
    }

    decInfo->extn_size = size;

    return e_success;
}

Status decode_secret_file_extn(DecodeInfo *decInfo)
{
    if (decode_data_from_image(decInfo->extn_secret_file, decInfo->extn_size, decInfo->fptr_stego_image) == e_failure)
    {
        return e_failure;
    }

    decInfo->extn_secret_file[decInfo->extn_size] = '\0';

    return e_success;
}

Status decode_secret_file_size(DecodeInfo *decInfo)
{
    int size;

    if (decode_data_from_image((char *)&size, sizeof(int), decInfo->fptr_stego_image) == e_failure)
    {
        return e_failure;
    }

    decInfo->size_secret_file = size;

    return e_success;
}

Status decode_secret_file_data(DecodeInfo *decInfo)
{
    char ch;
    char arr[8];

    for (long i = 0; i < decInfo->size_secret_file; i++)
    {
        if (fread(arr, 8, 1, decInfo->fptr_stego_image) != 1)
        {
            return e_failure;
        }

        decode_byte_from_lsb(&ch, arr);

        fputc(ch, decInfo->fptr_secret);
    }

    return e_success;
}

Status do_decoding(DecodeInfo *decInfo)
{
    if (open_files_decode(decInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to open files\n");
        return e_failure;
    }
    printf("INFO: Opened stego image file\n");

    if (decode_magic_string(decInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Magic string mismatch, not a valid stego image\n");
        return e_failure;
    }
    printf("INFO: Magic string matched\n");

    if (decode_secret_file_extn_size(decInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to decode extension size\n");
        return e_failure;
    }

    if (decode_secret_file_extn(decInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to decode extension\n");
        return e_failure;
    }
    printf("INFO: Decoded secret file extension : %s\n", decInfo->extn_secret_file);

    if (decode_secret_file_size(decInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to decode secret file size\n");
        return e_failure;
    }
    printf("INFO: Decoded secret file size : %ld\n", decInfo->size_secret_file);

    snprintf(decInfo->secret_fname_with_extn, MAX_SECRET_FNAME, "%s%s",
             decInfo->secret_fname, decInfo->extn_secret_file);

    decInfo->fptr_secret = fopen(decInfo->secret_fname_with_extn, "w");
    if (decInfo->fptr_secret == NULL)
    {
        perror("fopen");
        fprintf(stderr, "ERROR: Unable to open file %s\n", decInfo->secret_fname_with_extn);

        return e_failure;
    }

    if (decode_secret_file_data(decInfo) == e_failure)
    {
        fprintf(stderr, "ERROR: Failed to decode secret file data\n");
        return e_failure;
    }
    printf("INFO: Decoded secret file data successfully -> %s\n", decInfo->secret_fname_with_extn);

    fclose(decInfo->fptr_stego_image);
    fclose(decInfo->fptr_secret);

    return e_success;
}
