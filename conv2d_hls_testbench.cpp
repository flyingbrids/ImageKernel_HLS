#include "conv2d_hls.h"
#include <stdio.h>
#include <stdlib.h>


#define MAX_CHAR 256

int lineCnt = 0;

int main() 
{
    hls::stream<axis_t> output_stream;
    FILE *test_image;
    FILE *output_image;
    FILE *output_image1;
    const char *imagefile = "../../../../../imgr1_ref_image.hex";// Name of the file to read
    const char *imageout  = "../../../../../imgr1_ref_image.pgm";// Name of the file to write
    const char *imageout1  = "../../../../../imgr1_filter_image.pgm";// Name of the file to write
    char line_buffer[4];

    // Load grayscale image
    // 1. Open the file 
    test_image = fopen(imagefile, "r");
    output_image = fopen(imageout, "w");
    output_image1 = fopen(imageout1, "w");
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
    fprintf(output_image1,"P2\n%d    %d\n# CREATOR: SHEN\n1023\n",IMG_COLS,IMG_ROWS);
    // 3. Read line by line until end of file
    // fgets reads up to (MAX_CHAR - 1) characters or until a newline or EOF.
    // It includes the newline character if found.
    ap_uint<320> buffer;
    long input_arr[N];
    int n = 0;    
    while (lineCnt < IMG_ROWS * IMG_COLS) 
    {    
            for (int i = 0; i < 4; i++)
            {
                for (int k = 0; k < PIX_CNT; k++) 
                {
                    int pixeldata = 0;
                    uint8_t pixelNibble;
                    if (lineCnt >= IMG_ROWS * IMG_COLS)
                       break;
                    if (fgets(line_buffer, MAX_CHAR, test_image))
                    {
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
                        // write output image 
                        fprintf(output_image,"%d\r\n",pixeldata);
                        buffer.range(i*80+(PIX_CNT-1-k)*10+9 , i*80+(PIX_CNT-1-k)*10) = pixeldata; 
                        lineCnt ++;
                    }
                }
            }

            for (int k = 0; k < 5; k++) 
            {
                input_arr[n+k] = buffer.range(k*64 + 63, k*64);
            }
            n = n +5;                   
    
    }
    // 4. Close the file
    fclose(test_image);
    fclose(output_image);

    // Run hardware convolution
    conv2d_3x3 (input_arr, output_stream); 
    int  pixels_received =0;
    // get the output file 
    while (!output_stream.empty()) 
    {
        axis_t pixel_out;
        output_stream >> pixel_out; // Read a pixel from the output stream
        for (int k =PIX_CNT-1; k >=0; k--) 
        {
            fprintf(output_image1,"%d\r\n",(int)pixel_out.range(k*10+9,k*10)); 
            pixels_received++; 
        }        
    }
    fclose(output_image1);
    if (pixels_received == IMG_ROWS * IMG_COLS)
        return 0;
    else
        return -1;

}