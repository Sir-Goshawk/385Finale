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
	int x = 40;
	int y = 30;
	screenVGAinitializer();
	for (int i = 0; i <  60; i++) {
		VGADrawColorBox(9+i,9,15,0);
		VGADrawColorBox(9+i,48,15,0);
		setEdge(9+i,9);
		setEdge(9+i,48);
	}
	for (int i = 0; i < 40; i++) {
		VGADrawColorBox(9,9+i,15,0);
		VGADrawColorBox(68,9+i,15,0);
		setEdge(9,9+i);
		setEdge(68,9+i);
	}
	userControlledBlock(x, y, 1,3);
	keyInput(x, y, 1,3);
	return 1;
}


