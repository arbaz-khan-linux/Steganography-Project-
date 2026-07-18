# 📷 Image Steganography in C

A robust, low-level command-line tool written in C to securely hide secret text messages inside BMP images (Encoding) and extract them back (Decoding). This application directly parses BMP headers and manipulates image pixels using bitwise operations without relying on any external libraries.

---

## 🚀 Key Features

* **Secure Hiding (Encoding):** Embeds text files into the Least Significant Bits (LSB) of the image pixels.
* **Extraction (Decoding):** Retrieves the hidden text from the modified image seamlessly.
* **Low-Level File I/O:** Manual parsing of BMP headers (`54 bytes`) to ensure image integrity.
* **Error Handling:** Validates file sizes, formats, and guarantees that the carrier image has enough capacity for the payload.

---

## 📁 File Structure

* `main.c` - Entry point handling command-line arguments.
* `encode.c` / `encode.h` - Core logic for data encoding and LSB manipulation.
* `decode.c` / `decode.h` - Core logic for data decoding and text extraction.
* `types.h` / `common.h` - Custom type definitions and common configurations.

---

## 🛠️ How to Compile and Run

### 1. Compilation
Open your terminal inside the project directory and run the following command to compile the source files using `gcc`:

```bash
gcc main.c encode.c decode.c -o stego

### 2. Running the Application

* **To Encode (Hide Text):**
  ```bash
  ./stego -e beautiful.bmp secret.txt stego_img.bmp


To Decode (Extract Text):
./stego -d stego_img.bmp output.txt
