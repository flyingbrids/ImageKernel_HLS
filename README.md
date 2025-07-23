# ImageKernel_HLS
Instead using traditional RTL, this project possibility of using Vitis HLS 
This project requires Vitis Vision Library.

https://github.com/Xilinx/Vitis_Libraries

To compile and run the testbench, you will need to include the library in your syn.cflag and syn.csimflags. as follows.

syn.cflags=-I/path to your Vitis_Libraries/vision/L1/include -I/path to your/Vitis_Libraries/vision/L1/include/common
syn.csimflags=I/path to your Vitis_Libraries/vision/L1/include -I/path to your/Vitis_Libraries/vision/L1/include/common

Due to the version incompatibility, this testbench doesn't use openCV function to read and write the image file. Instead, it can read in the grayscale image file in the hex file format and output the result image in the .pgm format.

