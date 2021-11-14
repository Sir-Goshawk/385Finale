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
	for (int i = 0; i < 64; i++) {
		textVGADrawColorBox(9+i,9,15,0);
		textVGADrawColorBox(9+i,52,15,0);
	}
	for (int i = 0; i < 44; i++) {
		textVGADrawColorBox(9,9+i,15,0);
		textVGADrawColorBox(72,9+i,15,0);
	}
	userControlledBlock(x, y, 5,3);
	keyInput(x, y, 5,3);
	//textVGAColorScreenSaver();
	/*userControlledBlock(x, y, 1);
	userControlledBlock(0, 0, 2);
	userControlledBlock(x, 10, 3);
	userControlledBlock(10, y, 4);
	userControlledBlock(50, 50, 5);
	userControlledBlock(40, y, 6);
	userControlledBlock(70, 50, 7);*/
	return 1;
}


