#include <iostream>
#include <queue>
#include "conv2d_hls.h"
#include <cstdint>
#include <hls_stream.h>

#define IMG_ROWS 2048
#define IMG_COLS 2448
#define MAX_CHAR 256

int main() {
    hls::stream<pixel_t> input_stream, output_stream;
    FILE *test_image;
    FILE *output_image;
    const char *imagefile = "../../../../../imgr1_ref_image.hex";// Name of the file to read
    const char *imageout  = "../../../../../imgr1_ref_image.pgm";// Name of the file to write
    char line_buffer[4];

    // Load grayscale image
    // 1. Open the file 
    test_image = fopen(imagefile, "r");
    output_image = fopen(imageout, "w");

    // 2. Check if file opened successfully
    if (test_image == NULL) {
        perror("Error opening file"); // Prints a descriptive error message
        return EXIT_FAILURE;         // Indicate an error
    }    
    
    if (output_image == NULL) {
        perror("Error opening file"); // Prints a descriptive error message
        return EXIT_FAILURE;         // Indicate an error
    }   
    fprintf(output_image,"P2\n%d    %d\n# CREATOR: SHEN\n1023\n",IMG_COLS,IMG_ROWS);

    // 3. Read line by line until end of file
    // fgets reads up to (MAX_CHAR - 1) characters or until a newline or EOF.
    // It includes the newline character if found.
    while (fgets(line_buffer, MAX_CHAR, test_image) != NULL) 
    {           
            int pixeldata = 0;
            uint8_t pixelNibble;
            for (int index =0; index < 4; index++) 
        {
            if (line_buffer[index] < 'A')
            {
                pixelNibble = line_buffer[index] - '0';
            } // ASCII value to data      
            else
                pixelNibble = line_buffer[index] - 'A' + 10;
            switch (index)
            {
                case 0: pixeldata = pixeldata + (pixelNibble << 4); break;
                case 1: pixeldata = pixeldata +  pixelNibble; break;
                case 2: pixeldata = pixeldata + (pixelNibble << 12);  break;
                case 3: pixeldata = pixeldata + (pixelNibble << 8);  break;
                default: break;
            }
        }
        
        input_stream << ((pixel_t)pixeldata);
        // write output image 
        //fprintf(output_image,"%d\r\n",pixeldata);
    }

    // 4. Close the file
    fclose(test_image);

    // Run hardware convolution
    conv2d_3x3 (input_stream, output_stream);        

    int  pixels_received =0;
    // get the output file 
    while (!output_stream.empty()) 
    {
        pixel_t pixel_out;
        output_stream >> pixel_out; // Read a pixel from the output stream
        unsigned int debug = (unsigned int )pixel_out;
        fprintf(output_image,"%d\r\n",debug); 
        pixels_received++; 
    }
    fclose(output_image);
    if (pixels_received == IMG_ROWS * IMG_COLS)
        return 0;
    else
        return -1;
}
