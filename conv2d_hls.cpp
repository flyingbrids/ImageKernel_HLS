#include "conv2d_hls.h"
#include <cassert>
#include "common/xf_video_mem.hpp"

#define IMG_ROWS 2048
#define IMG_COLS 2448
#define PIX_CNT  8 // process 8 pixels at a time. 

void conv2d_3x3(hls::stream<pixel_t> &input, hls::stream<pixel_t> &output) {
#pragma HLS INTERFACE axis port=input
#pragma HLS INTERFACE axis port=output
#pragma HLS INTERFACE ap_ctrl_none port=return

    const int kernel[3][3] = {
        { -1,  0,  1 },
        { -2,  0,  2 },
        { -1,  0,  1 }
    };


    xf::cv::LineBuffer<3, IMG_COLS, pixel_t> linebuf; 
    pixel_t window[3][3];
    int col_adj;

#pragma HLS ARRAY_PARTITION variable=kernel complete dim=0
#pragma HLS ARRAY_PARTITION variable=window complete dim=0

    for (int row = 0; row < IMG_ROWS + 2; row++)
    {
        for (int col = 0; col < IMG_COLS + 2; col++) 
        {
            #pragma HLS PIPELINE 

            pixel_t new_pixel = 0;
            if (row < IMG_ROWS && col < IMG_COLS) 
            {
                new_pixel = input.read();
                linebuf.shift_up(col);
                linebuf.insert_bottom_row(new_pixel, col);
            }
            
            if (row >= 2 && col >= 2)
            {
                int sum = 0;
                for (int i = 0; i < 3; i++)
                 {
                    #pragma HLS unroll
                    for (int j = 0; j < 3; j++)
                     {
                        #pragma HLS unroll
                        if (col== IMG_COLS + 1 && j==2)  
                            col_adj = col - 2;
                        else if ((col== IMG_COLS + 1 && j==1) || (col== IMG_COLS && j==2) ) 
                            col_adj = col - 1;
                        else  
                            col_adj = col;

                        window[i][j] = linebuf.getval(i, col_adj - 2 + j);
                        sum += window[i][j] * kernel[i][j];                        
                    }
                }
                if (sum < 0) sum = 0;
                if (sum > 1023) sum = 1023;
                output.write((pixel_t)sum); 
            }
        }
    }
}

