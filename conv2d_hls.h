#include <hls_stream.h>
#include <ap_int.h>

#define IMG_ROWS 2048
#define IMG_COLS 2448
#define DEPTH_CPP IMG_COLS * 2 // Example depth of the shift register
#define PIX_CNT  8 // process 8 pixels at a time. 

typedef ap_uint<10> pixel_t;
typedef ap_uint<80> axis_t;
void conv2d_3x3(hls::stream<axis_t> &input, hls::stream<axis_t> &output);
