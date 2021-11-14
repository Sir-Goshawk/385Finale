/*
 *  text_mode_vga.h
 *	Minimal driver for text mode VGA support, ECE 385 Summer 2021 Lab 6
 *  You may wish to extend this driver for your final project/extra credit project
 * 
 *  Created on: Jul 17, 2021
 *      Author: zuofu
 */

#ifndef TEXT_MODE_VGA_COLOR_H_
#define TEXT_MODE_VGA_COLOR_H_

#define COLUMNS 80
#define ROWS 60

#include <system.h>
#include <alt_types.h>

struct TEXT_VGA_STRUCT {
	alt_u8 VRAM [ROWS*COLUMNS*2]; //Week 2 - extended VRAM
	//modify this by adding const bytes to skip to palette, or manually compute palette
	//alt_u8 reserve [(0x1FFF-0x12C0)];
	alt_u8 reserve [(0x2FFF-0x2581)];
	alt_u32 palette [8];
	//alt_u8 blank [(0x3FFF-0x2020)];
	alt_u8 blank [(0x3FFF-0x3020)];
};

struct COLOR{
	char name [20];
	alt_u8 red;
	alt_u8 green;
	alt_u8 blue;
};


//you may have to change this line depending on your platform designer
static volatile struct TEXT_VGA_STRUCT* vga_ctrl = VGA_TEXT_MODE_CONTROLLER_0_BASE;

//CGA colors with names
static struct COLOR colors[]={
    {"black",          0x0, 0x0, 0x0},//0
	{"blue",           0x0, 0x0, 0xa},//1
    {"green",          0x0, 0xa, 0x0},//2
	{"cyan",           0x0, 0xa, 0xa},//3
    {"red",            0xa, 0x0, 0x0},//4
	{"magenta",        0xa, 0x0, 0xa},//5
    {"brown",          0xa, 0x5, 0x0},//6
	{"light gray",     0xa, 0xa, 0xa},//7
    {"dark gray",      0x5, 0x5, 0x5},//8
	{"light blue",     0x5, 0x5, 0xf},//9
    {"light green",    0x5, 0xf, 0x5},//10
	{"light cyan",     0x5, 0xf, 0xf},//11
    {"light red",      0xf, 0x5, 0x5},//12
	{"light magenta",  0xf, 0x5, 0xf},//13
    {"yellow",         0xf, 0xf, 0x5},//14
	{"white",          0xf, 0xf, 0xf}//15
};


void textVGAColorClr();
void textVGADrawColorText(char* str, int x, int y, alt_u8 background, alt_u8 foreground);
void setColorPalette (alt_u8 color, alt_u8 red, alt_u8 green, alt_u8 blue); //Fill in this code

void textVGAColorScreenSaver(); //Call this for your demo
void screenVGAinitializer();
void userControlledBlock(int x, int y, int index, int color);

#endif /* TEXT_MODE_VGA_COLOR_H_ */
