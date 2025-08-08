#include "conv2d_hls.h"
#include <cassert>
#include "common/xf_video_mem.hpp"

const int kernel[3][3] = {
        { -1,  0,  1 },
        { -2,  0,  2 },
        { -1,  0,  1 }
};
/*
const int kernel[3][3] = {
        { 0,  0,  0 },
        { 0,  1,  0 },
        { 0,  0,  0 }
};
*/
xf::cv::LineBuffer<3, IMG_COLS/PIX_CNT, axis_t> linebuf; 

pixel_t window_adjust[3][3][PIX_CNT];
pixel_t window[3][3][PIX_CNT];
axis_t  line[3][3];
int sum[PIX_CNT];

axis_t sobelConvolve () 
{
   #pragma HLS ARRAY_PARTITION variable=kernel complete dim=0
   axis_t data_out = 0;
   // calculate the convolution
    for (int k = 0; k < PIX_CNT; k++) 
    {
        sum[k] = 0;
        for (int i = 0; i < 3; i++)
        {
            #pragma HLS UNROLL
            for (int j = 0; j < 3; j++)
            {
                #pragma HLS UNROLL
                sum[k] += window_adjust[i][j][k] * kernel[i][j];                        
            }
        }
        if (sum[k] < 0) 
            sum[k] = 0;
        else if (sum[k] > 1023) 
            sum[k] = 1023;
        data_out = (data_out << 10) + sum[k];
    }
    return data_out;   
} 

axis_t shift_register (axis_t d, int col, int row) 
{
    #pragma HLS ARRAY_PARTITION variable=window complete dim=0
    #pragma HLS ARRAY_PARTITION variable=window_adjust complete dim=0
    // inject to the line buffer
    if (col < IMG_COLS/PIX_CNT) 
    {
        linebuf.shift_up(col);
        linebuf.insert_bottom_row(d, col);
    }

    // populate and shift line window, handle 4 edges
    if (col > 0) 
    {
        for (int i=0; i<3; i++) 
        {
           // Array Assignments for Sweeping window:
// ---------------------------------------------------------------------------------
//     line0                    line1                       line2
//  _______________________  _______________________  _______________________
// |__|__|__|__|__|__|__|__||__|__|__|__|__|__|__|__||__|__|__|__|__|__|__|__|
//  7  6  5  4  3  2  1  0   7  6  5  4  3  2  1  0   7  6  5  4  3  2  1  0
//                       \_____________________________/ 
//    pix array          |__|__|__|__|__|__|__|__|__|__|             
//                        9  8  7  6  5  4  3  2  1  0  
//
//                               8x window sweep
//                        ________ 
//               k=0     |__|__|__| ---->  ---->  ---->|
//                        a  b  c  
//                           ________
//               k=1        |__|__|__| -->  ----> ---->|
//                           a  b  c
//                              ...            ________
//               k=7                          |__|__|__|                                                     
//                                             a  b  c
// ---------------------------------------------------------------------------------
            #pragma HLS UNROLL 
            line[i][0] = (col == 1)? linebuf.getval(i, 0) : linebuf.getval(i, col-2);   
            line[i][1] = linebuf.getval(i, col-1);
            line[i][2] = (col == IMG_COLS/PIX_CNT)? linebuf.getval(i, IMG_COLS/PIX_CNT-1) : linebuf.getval(i, col); 
            for (int j=0; j<3; j++) 
            {
                #pragma HLS UNROLL 
                for (int k =0; k<PIX_CNT; k++) 
                {
                   #pragma HLS UNROLL 
                   if (k ==0 & j == 0)
                       window[i][j][k] = (pixel_t)line[i][0].range(9, 0);
                   else if (k ==7 & j == 2)
                       window[i][j][k] = (pixel_t)line[i][2].range(79,70);
                   else
                       window[i][j][k] = (pixel_t)line[i][1].range((PIX_CNT-k-j)*10+9, (PIX_CNT-k-j)*10);
                }                
            }
        }
                                                                     
       
        for (int j = 0; j < 3; j++) 
        {
            #pragma HLS UNROLL 
            for (int k = 0; k < PIX_CNT; k++) 
            {
                #pragma HLS UNROLL
                window_adjust[0][j][k] = (row == 1)? window[1][j][k] : window[0][j][k];  
                window_adjust[1][j][k] = window[1][j][k];
                window_adjust[2][j][k] = (row == IMG_ROWS)? window[1][j][k] : window[2][j][k];
            }                                            
        }
    }

    // perform convolution
    return sobelConvolve();
}



void conv2d_3x3(hls::stream<axis_t> &input, hls::stream<axis_t> &output) {
    #pragma HLS INTERFACE axis port=input
    #pragma HLS INTERFACE axis port=output
    #pragma HLS INTERFACE ap_ctrl_none port=return  
    int row = 0; 
    int col = 0;  
    axis_t data_out;
    do {
        #pragma HLS PIPELINE
        do{
            #pragma HLS PIPELINE II = 1
            if (!input.empty())
            {                     
                data_out = shift_register (input.read(), col, row);                  
                if (row > 0 && col > 0) 
                {
                    output.write(data_out);  
                }                
                col ++ ; 
            }

            else if (row == IMG_ROWS)
            {
              data_out = shift_register (0, col, row);
              if (col > 0)
              {
                  output.write(data_out);  
              }              
              col ++ ; 
            }     
        }
        while (col < IMG_COLS/PIX_CNT);        

       // handle when (col == IMG_COLS/PIX_CNT)
        data_out = shift_register (0, col, row);
        if (row > 0) 
        {
            output.write(data_out); 
        }            
        col = 0;
        row ++;
    }
    while (row <= IMG_ROWS);
 }



 
                       

                



