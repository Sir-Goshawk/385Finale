/*
 * text_mode_vga_color.c
 * Minimal driver for text mode VGA support
 * This is for Week 2, with color support
 *
 *  Created on: Oct 25, 2021
 *      Author: zuofu
 */

#include <system.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alt_types.h>
#include "text_mode_vga_color.h"

void textVGAColorClr()
{
	for (int i = 0; i<(ROWS*COLUMNS) * 2; i++)
	{
		vga_ctrl->VRAM[i] = 0x00;
	}
}

void textVGADrawColorText(char* str, int x, int y, alt_u8 background, alt_u8 foreground)
{
	int i = 0;
	while (str[i]!=0)
	{
		vga_ctrl->VRAM[(y*COLUMNS + x + i) * 2] = foreground << 4 | background;
		vga_ctrl->VRAM[(y*COLUMNS + x + i) * 2 + 1] = str[i];
		i++;
	}
}

void setColorPalette (alt_u8 color, alt_u8 red, alt_u8 green, alt_u8 blue)
{
	alt_u32 shifts;
	alt_u32 original;
	original = vga_ctrl->palette[color/2];
	shifts = red;
	shifts = shifts << 4;
	shifts = shifts | green;
	shifts = shifts << 4;
	shifts = shifts | blue;
	if (color %2 ==0){
		original = original & 0xFFFFE000;
		shifts = shifts << 1;
	} else {
		original = original & 0x00001FFF;
		shifts = shifts << 13;
	}
	original = original | shifts;
	vga_ctrl->palette[color/2] = original;
}

void screenVGAinitializer() {
	textVGAColorClr();
	//initialize palette
	for (int i = 0; i < 16; i++)
	{
		setColorPalette (i, colors[i].red, colors[i].green, colors[i].blue);
	}

}

void textVGAColorScreenSaver()
{
	//This is the function you call for your week 2 demo
	int flag = 0;
	char color_string[80];
    int fg, bg, x, y;
	textVGAColorClr();
	//initialize palette
	for (int i = 0; i < 16; i++)
	{
		setColorPalette (i, colors[i].red, colors[i].green, colors[i].blue);
	}
	int count =0;
	while (1) {
	//for(int y = 0; y < COLUMNS; y++){
		fg = rand() % 16;
		bg = rand() % 16;
		while (fg == bg)
		{
			fg = rand() % 16;
			bg = rand() % 16;
		}
		//sprintf(color_string," ttttttt ");
		sprintf(color_string, "Drawing %s text with %s background", colors[fg].name, colors[bg].name);
		x = rand() % (ROWS-strlen(color_string));
		y = rand() % 60;
		if (y %2==0) {
			textVGADrawColorText (color_string, 20, y, bg, fg);

		}
		usleep (100000);
	//}
	}
	//VGADrawColorBox(2,2,15,0);
	//VGADrawColorBox(ROWS/2,COLUMNS/2,3,0);
}

void VGADrawColorBox(int x, int y, alt_u8 background)
{
	vga_ctrl->VRAM[(y*COLUMNS + x) * 2] = 0 << 4 | background;
}

void userControlledBlock(int x, int y, int index, int color) {
	switch(index) {
	   case 0  :
		   VGADrawColorBox(x,y-3,color);
		   VGADrawColorBox(x,y-2,color);
		   VGADrawColorBox(x,y-1,color);
		   VGADrawColorBox(x,y,color);
	      break;
	   case 1  :
		   VGADrawColorBox(x+1,y-3,color);
		   VGADrawColorBox(x,y-3,color);
		   VGADrawColorBox(x,y-2,color);
		   VGADrawColorBox(x,y-1,color);
		   VGADrawColorBox(x,y,color);
	      break;
	   case 2  :
		   VGADrawColorBox(x+1,y,color);
		   VGADrawColorBox(x,y-3,color);
		   VGADrawColorBox(x,y-2,color);
		   VGADrawColorBox(x,y-1,color);
		   VGADrawColorBox(x,y,color);
	      break;
	   case 3  :
		   VGADrawColorBox(x,y,color);
		   VGADrawColorBox(x-1,y,color);
		   VGADrawColorBox(x+1,y,color);
		   VGADrawColorBox(x,y-1,color);
	      break;
	   case 4  :
		   VGADrawColorBox(x,y,color);
		   VGADrawColorBox(x+1,y,color);
		   VGADrawColorBox(x+1,y-1,color);
		   VGADrawColorBox(x,y-1,color);
	      break;
	   case 5  :
		   VGADrawColorBox(x,y,color);
		   VGADrawColorBox(x+1,y,color);
		   VGADrawColorBox(x,y-1,color);
		   VGADrawColorBox(x-1,y-1,color);
	      break;
	   default :
		   VGADrawColorBox(x,y,color);
		   VGADrawColorBox(x,y-1,color);
		   VGADrawColorBox(x+1,y-1,color);
		   VGADrawColorBox(x+1,y,color);
	}
}
