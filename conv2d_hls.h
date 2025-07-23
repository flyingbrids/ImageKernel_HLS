#include <hls_stream.h>
#include <ap_int.h>

typedef ap_uint<10> pixel_t;
void conv2d_3x3(hls::stream<pixel_t> &input, hls::stream<pixel_t> &output);
