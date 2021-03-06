/************************************************************************
Avalon-MM Interface VGA Text mode display

Register Map:
0x000-0x0257 : VRAM, 80x30 (2400 byte, 600 word) raster order (first column then row)
0x258        : control register

VRAM Format:
X->
[ 31  30-24][ 23  22-16][ 15  14-8 ][ 7    6-0 ]
[IV3][CODE3][IV2][CODE2][IV1][CODE1][IV0][CODE0]

IVn = Draw inverse glyph
CODEn = Glyph code from IBM codepage 437

Control Register Format:
[[31-25][24-21][20-17][16-13][ 12-9][ 8-5 ][ 4-1 ][   0    ]
[[RSVD ][FGD_R][FGD_G][FGD_B][BKG_R][BKG_G][BKG_B][RESERVED]

VSYNC signal = bit which flips on every Vsync (time for new frame), used to synchronize software
BKG_R/G/B = Background color, flipped with foreground when IVn bit is set
FGD_R/G/B = Foreground color, flipped with background when Inv bit is set

************************************************************************/
`define NUM_REGS 601 //80*30 characters / 4 characters per register
`define CTRL_REG 600 //index of control register

module vga_text_avl_interface (
	// Avalon Clock Input, note this clock is also used for VGA, so this must be 50Mhz
	// We can put a clock divider here in the future to make this IP more generalizable
	input logic CLK,

	// Avalon Reset Input
	input logic RESET,

	// Avalon-MM Slave Signals
	input  logic AVL_READ,					// Avalon-MM Read
	input  logic AVL_WRITE,					// Avalon-MM Write
	input  logic AVL_CS,					// Avalon-MM Chip Select
	input  logic [3:0] AVL_BYTE_EN,			// Avalon-MM Byte Enable
	input  logic [12:0] AVL_ADDR,			// Avalon-MM Address
	input  logic [31:0] AVL_WRITEDATA,		// Avalon-MM Write Data
	output logic [31:0] AVL_READDATA,		// Avalon-MM Read Data

	// Exported Conduit (mapped to VGA port - make sure you export in Platform Designer)
	output logic [3:0]  red, green, blue,	// VGA color channels (mapped to output pins in top-level)
	output logic hs, vs						// VGA HS/VS
);

logic [31:0] reg_32 [8]; // Registers


//put other local variables here
logic [9:0] DrawX, DrawY;
logic blank, sync, pixel_clk, inCurrent;
logic [7:0] fontRomOutput;
logic [31:0] registerOutput, controlOut;
logic [6:0] posX, posY, yAccess;
logic [12:0] VRAMAddress;
//wk 2 variables
logic [15:0] charCurrent;
logic [3:0] colorRegB, colorRegF, rBack, gBack, bBack, rFore, gFore, bFore, yAdjust;
logic [31:0] colorOutB, colorOutF;

//Declare submodules..e.g. VGA controller, ROMS, etc
vga_controller vgaController(.Clk(CLK),.Reset(RESET),.hs(hs),.vs(vs),.pixel_clk(pixel_clk),.blank(blank),.sync(sync),.DrawX(DrawX),.DrawY(DrawY));


always_comb //access characterss
begin
	posX = DrawX[9:3]; //drawX/8
	posY = DrawY[9:3]; //drawY/8
	yAdjust = DrawY[3:0];
	//if (DrawY[0] == 1) yAdjust = yAdjust +1; 
	VRAMAddress = posY*80+posX;	 //wk2
end

ram accessRAM(.address_a(AVL_ADDR[11:0]),
.address_b(VRAMAddress[12:1]), //VRAM/2 ever register has 2 chara
.byteena_a(AVL_BYTE_EN),
.clock(CLK),
.data_a(AVL_WRITEDATA), .data_b(31'b0),
.rden_a(AVL_READ), .rden_b(1'b1),
.wren_a(AVL_WRITE), .wren_b(1'b0),
.q_a(AVL_READDATA), .q_b(registerOutput));

always_ff @(posedge CLK)
begin
	if (AVL_CS == 1'b1 && (AVL_ADDR[11] == 1'b1 && AVL_ADDR[10] == 1'b1))
		begin
			if (AVL_WRITE == 1'b1 && AVL_ADDR >= 4'h3000)
			begin
				case (AVL_BYTE_EN)
					4'b1111 : reg_32[AVL_ADDR[4:0]] <= AVL_WRITEDATA;
					4'b1100 : reg_32[AVL_ADDR[4:0]][31:16] <= AVL_WRITEDATA[31:16];
					4'b0011 : reg_32[AVL_ADDR[4:0]][15:0] <= AVL_WRITEDATA[15:0];
					4'b1000 : reg_32[AVL_ADDR[4:0]][31:24] <= AVL_WRITEDATA[31:24];
					4'b0100 : reg_32[AVL_ADDR[4:0]][23:16] <= AVL_WRITEDATA[23:16];
					4'b0010 : reg_32[AVL_ADDR[4:0]][15:8] <= AVL_WRITEDATA[15:8];
					4'b0001 : reg_32[AVL_ADDR[4:0]][7:0] <= AVL_WRITEDATA[7:0];
				endcase
			end
		end

	colorOutB <= reg_32[colorRegB[3:1]];
	colorOutF <= reg_32[colorRegF[3:1]];
end


always_comb
begin
	unique case(VRAMAddress[0])
		1'b1: charCurrent = registerOutput[31:16]; //if it's 1, use MSB chara
		1'b0: charCurrent = registerOutput[15:0]; //if it's 0, use LSB chara
	endcase
	inCurrent = charCurrent[15]; //MSB invert
	colorRegB = charCurrent[3:0]; //set background code
	colorRegF = charCurrent[7:4]; //set forground code
	if (posY[0] == 1'b1)
		yAccess = DrawY[3:0] - 1;
	else
		yAccess = DrawY[3:0];
end

font_rom fontRom(.addr({charCurrent[14:8],DrawY[3:0]}),.data(fontRomOutput));


//handle drawing (may either be combinational or sequential - or both).

always_ff @(posedge pixel_clk)
begin
	if(blank == 1'b0)
	begin
		red <= 4'b0;
		green <= 4'b0;
		blue <= 4'b0;
	end
	else
	begin
		unique case(colorRegB[0])
			1'b1: //if it's 1, use MSB chara
			begin
				rBack <= colorOutB[24:21];
				gBack <= colorOutB[20:17];
				bBack <= colorOutB[16:13];
			end
			1'b0: //if it's 0, use LSB chara
			begin
				rBack <= colorOutB[12:9];
				gBack <= colorOutB[8:5];
				bBack <= colorOutB[4:1];
			end
		endcase
		unique case(colorRegF[0])
			1'b1: //if it's 1, use MSB chara
			begin
				rFore <= colorOutF[24:21];
				gFore <= colorOutF[20:17];
				bFore <= colorOutF[16:13];
			end
			1'b0: //if it's 0, use LSB chara
			begin
				rFore <= colorOutF[12:9];
				gFore <= colorOutF[8:5];
				bFore <= colorOutF[4:1];
			end
		endcase

		if (fontRomOutput[7-DrawX[2:0]] != inCurrent)  //wk2\
		begin
			red <= rFore;
			green <= gFore;
			blue <= bFore;
		end
		else
		begin
			red <= rBack;
			green <= gBack;
			blue <= bBack;
		end
	end

end


endmodule
