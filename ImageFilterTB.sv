`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 07/24/2025 08:21:02 AM
// Design Name: 
// Module Name: ImageFilterTB
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////
module ImageFilterTB();


localparam FRAME_CNT    = 1;   // # of test image 
localparam CLOCK_FREQ   = 200; // MHz
localparam CLOCK_PERIOD = (1000ns/CLOCK_FREQ);
localparam DWIDTH       = 16;

bit sys_clk;
bit sys_rst;
bit new_frame;
int frame_cnt;
bit input_r_TREADY;

logic [31:0] START_ROW;
logic [31:0] STOP_ROW;
logic [31:0] ROW_NUM;
logic [31:0] COL_NUM;
logic [31:0] PIX_PER_SLOT;
logic [31:0] LANE_N;

initial sys_clk = 1'b0;
always #(CLOCK_PERIOD/2) sys_clk = ~sys_clk;

task wait_for_reset;
  @(posedge sys_clk);
  @(posedge sys_clk);
  sys_rst = 1'b1; 
  @(posedge sys_clk);
  @(posedge sys_clk);
  @(posedge sys_clk);  
  @(posedge sys_clk);  
  sys_rst = 1'b0;
  @(posedge sys_clk);
  @(posedge sys_clk);
  @(posedge sys_clk);
  $display("<<TESTBENCH NOTE>> system clk came out of reset");
endtask

task wait_for_new_frame;
  @(posedge sys_clk);
  new_frame = 1'b1;
  @(posedge sys_clk);
  new_frame = 1'b0;
  @(posedge sys_clk);
  @(posedge sys_clk);
  @(posedge sys_clk);
  @(posedge sys_clk);
  $display("<<TESTBENCH NOTE>> frame %0d requested", frame_cnt+1);
endtask

function int convert2Pixel (string pixel); // covert image.hex file into 10 bits pixel data
  int pixeldata;
  int pixelNibble;
  pixeldata = 0;
  for (int index =0; index < 4; index++) begin
    if (index == 2)
      continue;
    if ((pixel[index] >8'h29) & (pixel[index] < 8'h40)) begin // ASCII value to data
      pixelNibble = pixel[index] - 8'h30;
    end else
      pixelNibble = pixel[index] - 8'h37;
    case (index)
      0: pixeldata = pixeldata + (pixelNibble << 4);
      1: pixeldata = pixeldata +  pixelNibble;
      3: pixeldata = pixeldata + (pixelNibble << 8);	  
    endcase
  end
  return pixeldata;
endfunction

logic [DWIDTH-1:0]   imageData ;
logic                imageDataVld;
logic [DWIDTH-1:0]   filt_3x3_data ;
logic                filt_3x3_vld;

task automatic load_image;
  int i,j,k,d,t;
  int img;
  string pixel;
  begin
    // file process
    img  = $fopen($sformatf("imgr%0d.pgm",frame_cnt+1),"w");
    $fwrite(img,"P2\n%d%d\n# CREATOR: Shen\n1023\n",COL_NUM,ROW_NUM);
    @(posedge sys_clk);  
    for (i = START_ROW; i < STOP_ROW; i++) begin
        for (j=0; j<COL_NUM; j++) begin
            $fgets(pixel,file);
            imageData = convert2Pixel(pixel);
            imageDataVld = 1'b1;             
            $fwrite(img,"%d\n",imageData); 
            @(posedge sys_clk); 
            if (input_r_TREADY == 0) begin
                wait (input_r_TREADY);
                @(posedge sys_clk);    
            end      
         end
    $display("<<TESTBENCH NOTE>> image row %d is captured!",i);
    end
    $display("<<TESTBENCH NOTE>> raw image captured!");
    $fclose(img);
  end
endtask
    
    
conv2d_3x3  DUT 
(
        .ap_clk         (sys_clk),
        .ap_rst_n       (~sys_rst),
        .input_r_TVALID (imageDataVld),
        .output_r_TREADY(1'b1),
        .input_r_TDATA  (imageData),
        .input_r_TREADY (input_r_TREADY),
        .output_r_TDATA (filt_3x3_data),
        .output_r_TVALID(filt_3x3_vld)
); 

// filter 3x3 
task automatic capture_3x3;
int x=START_ROW; 
int y=0;
int img ;
  begin
   img = $fopen($sformatf("imgr%0d_3x3filter.pgm",frame_cnt+1),"w");
   $fwrite(img,"P2\n%d%d\n# CREATOR: Shen\n1023\n",COL_NUM,ROW_NUM);    
   @(posedge filt_3x3_vld)  
      do begin
        do begin
          @(posedge sys_clk);
           if(filt_3x3_vld ) begin             
               $fwrite(img,"%d\n",filt_3x3_data);             
               y=y+1;
            end
         end while (y<COL_NUM);
        y=0;
        x=x+1;
      end while (x<STOP_ROW);     
    $display("<<TESTBENCH NOTE>> 3x3 Filter succesfully captured!"); 
    $fclose(img);
  end
endtask

// main function
int file;
initial begin
  wait_for_reset();
  for (frame_cnt = 0; frame_cnt < FRAME_CNT; frame_cnt++) begin
      file = $fopen($sformatf("imgr%0d_ref_image.hex",frame_cnt+1),"r");
      if (!file)
         continue;
    START_ROW     = 0;
    STOP_ROW      = 2048;
    ROW_NUM       = STOP_ROW - START_ROW;
	 PIX_PER_SLOT  = 306;
	 LANE_N        = 8;
    COL_NUM       = PIX_PER_SLOT * LANE_N;
	 wait_for_new_frame();
	 fork
	    load_image();
		 capture_3x3();
	 join
  end
  $stop();
end  

    
endmodule
