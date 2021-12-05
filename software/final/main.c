/*
 * main.c
 *
 *  Created on: Nov 5, 2021
 *      Author: kelvin3
 */

#define COLUMNS 80
#define ROWS 60
#include "text_mode_vga_color.h"

int main() {
	screenVGAinitializer();
	for (int i = 0; i < 80; i++) {
		VGADrawColorBox(i,0,15);
		VGADrawColorBox(i,59,15);
	}
	for (int i = 0; i < 60; i++) {
		VGADrawColorBox(0,i,15);
		VGADrawColorBox(79,i,15);
	}
	for (int i = 20; i < 20+10; i++) {
		for (int j = 10; j < 10+24; j++) {
			VGADrawColorBox(i,j,11);
			VGADrawColorBox(i,j,11);
		}
	}


	VGAwriteText(10,15,2,0,"TETRIS");
	keyInput();
	return 1;
}
