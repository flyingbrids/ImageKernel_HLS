#include <hls_stream.h>
#include <ap_int.h>

#define IMG_ROWS 2048
#define IMG_COLS 2448
#define DEPTH_CPP IMG_COLS * 2 // Example depth of the shift register

typedef ap_uint<10> pixel_t;
void conv2d_3x3(hls::stream<pixel_t> &input, hls::stream<pixel_t> &output);
