#include "conv2d_hls.h"
#include <cassert>
#include "common/xf_video_mem.hpp"

#define PIX_CNT  8 // process 8 pixels at a time. 

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
xf::cv::LineBuffer<3, IMG_COLS, pixel_t> linebuf; 

pixel_t window_adjust[3][3];
pixel_t window[3][3];


void shift_register (pixel_t d, int col) 
{
    #pragma HLS ARRAY_PARTITION variable=window complete dim=0
    
    /* Need to use line buffer provided by Vitis libray instead of creating
       your own. 
    static pixel_t fifo[DEPTH_CPP];
    #pragma HLS ARRAY_PARTITION variable=fifo complete dim=1
    for (int i = DEPTH_CPP-2; i < DEPTH_CPP; i++) 
    {
        #pragma HLS PIPELINE II=1
        fifo[i] = (i == DEPTH_CPP -1)? d : fifo[i + 1];
    }
    */
    linebuf.shift_up(col);
    linebuf.insert_bottom_row(d, col);
    // populate and shift line window
    for (int i = 0; i < 3; i++)
    {
       #pragma HLS UNROLL
       for (int j = 1; j >=0; j--)               
       {
         #pragma HLS UNROLL
         window[i][j] = window[i][j+1];                
       }
    }
    window[0][2] = linebuf.getval(0, col);
    window[1][2] = linebuf.getval(1, col);
    window[2][2] = linebuf.getval(2, col);
}

int sobelConvolve () 
{
   #pragma HLS ARRAY_PARTITION variable=kernel complete dim=0
   // calculate the convolution
    int sum = 0;
    for (int i = 0; i < 3; i++)
    {
       #pragma HLS UNROLL
       for (int j = 0; j < 3; j++)
       {
         #pragma HLS UNROLL
         sum += window_adjust[i][j] * kernel[i][j];                        
        }
    }
    if (sum < 0) 
        return 0;
    else if (sum > 1023) 
        return 1023;
    else 
        return sum;
} 

void conv2d_3x3(hls::stream<pixel_t> &input, hls::stream<pixel_t> &output) {
    #pragma HLS INTERFACE axis port=input
    #pragma HLS INTERFACE axis port=output
    #pragma HLS INTERFACE ap_ctrl_none port=return     
    #pragma HLS ARRAY_PARTITION variable=window_adjust complete dim=0

    int row = 0; 
    int col = 0;  
    do {
        #pragma HLS PIPELINE
        do{
            #pragma HLS PIPELINE II = 1
            if (!input.empty())
            {     

                shift_register(input.read(), col);   

                // adjust window for edge bypass
                if (row == 1 && col > 0) 
                {
                   for (int j = 0; j <3; j++) 
                   {
                       #pragma HLS UNROLL
                       window_adjust[0][j]= window[1][1];
                       for (int i = 1; i < 3; i++)
                       {
                        #pragma HLS UNROLL
                        window_adjust[i][j]= (col == 1 && j ==0) ?window[1][1] : window[i][j];
                       }
                   }
                   output.write((pixel_t)sobelConvolve()); 
                } 

                else if (row > 1 && col > 0)
                {
                   for (int j = 0; j <3; j++) 
                   {
                       #pragma HLS UNROLL                       
                       for (int i = 0; i < 3; i++)
                       {
                        #pragma HLS UNROLL
                        window_adjust[i][j]= (col == 1 && j ==0) ?window[1][1] : window[i][j];
                       }
                   } 
                   output.write((pixel_t)sobelConvolve());  
                }
                
                col ++ ; 
            }

            else if (row == IMG_ROWS)
            {
                shift_register(0, col);  
                if (col > 0) 
                {
                    for (int j = 0; j <3; j++) 
                    {
                       #pragma HLS UNROLL
                       window_adjust[2][j]= window[1][1];
                       for (int i = 0; i < 2; i++)
                       {
                        #pragma HLS UNROLL
                        window_adjust[i][j]= (col == 1 && j ==0) ?window[1][1] : window[i][j];
                       }
                    }   
                    output.write((pixel_t)sobelConvolve());                 
                }
                
                col ++ ; 
            }   
                  
        }
        while (col < IMG_COLS);
        

       // adjust window for right edge of the image (col == IMG_COL)
        if (row == 1) 
        {
          for (int j = 0; j <3; j++) 
          {
                #pragma HLS UNROLL
                window_adjust[0][j]= window[1][1];
                for (int i = 1; i < 3; i++)
                {
                    #pragma HLS UNROLL
                    window_adjust[i][j]= (j == 2) ? window[1][1] : window[i][j+1];
                }
          }
          output.write((pixel_t)sobelConvolve());
        } 

        else if (row == IMG_ROWS)
        {      
            for (int j = 0; j <3; j++) 
            {
              #pragma HLS UNROLL
              window_adjust[2][j]= window[1][1];
              for (int i = 0; i < 2; i++)
              {
                #pragma HLS UNROLL
                window_adjust[i][j]= (j == 2) ? window[1][1] : window[i][j+1];
              }
            }
            output.write((pixel_t)sobelConvolve());            
        } 
        
        else if (row > 1)
        {
          for (int j = 0; j <3; j++) 
          {
            #pragma HLS UNROLL                       
            for (int i = 0; i < 3; i++)
            {
               #pragma HLS UNROLL
               window_adjust[i][j]= (j == 2) ? window[1][1] : window[i][j+1];
            }
          } 
          output.write((pixel_t)sobelConvolve()); 
        }   
         
        col = 0;
        row ++;
    }
    while (row <= IMG_ROWS);
 }



 
                       

                



