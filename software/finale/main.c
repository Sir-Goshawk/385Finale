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
	for (int i = 0; i <= 10; i++) {
		//VGADrawColorBox(9+i,9,15);
		setColor(36+i,9,15);
		//VGADrawColorBox(9+i,48,15);
		setColor(36+i,48,15);
		setEdge(36+i,9);
		setEdge(36+i,48);
	}
	for (int i = 0; i < 40; i++) {
		//VGADrawColorBox(9,9+i,15);
		//VGADrawColorBox(68,9+i,15);
		setColor(36,9+i,15);
		setColor(47,9+i,15);
		setEdge(35,9+i);
		setEdge(47,9+i);
	}
	userControlledBlockGrid(x, y, 4, 3);
	userControlledBlockGrid(1,13,1,5);
	userControlledBlockGrid(30,11,6,2);
	keyInput(x, y, 1,3);
	return 1;
}


