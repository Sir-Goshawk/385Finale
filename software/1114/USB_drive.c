/*
 * USB_drive.c
 *
 *  Created on: Nov 12, 2021
 *      Author: kelvin3
 */

#include <stdio.h>
#include "system.h"
#include "altera_avalon_spi.h"
#include "altera_avalon_spi_regs.h"
#include "altera_avalon_pio_regs.h"
#include "sys/alt_irq.h"
#include "usb_kb/GenericMacros.h"
#include "usb_kb/GenericTypeDefs.h"
#include "usb_kb/HID.h"
#include "usb_kb/MAX3421E.h"
#include "usb_kb/transfer.h"
#include "usb_kb/usb_ch9.h"
#include "usb_kb/USB.h"

extern HID_DEVICE hid_device;

static BYTE addr = 1; 				//hard-wired USB address
const char* const devclasses[] = { " Uninitialized", " HID Keyboard", " HID Mouse", " Mass storage" };
//the bottom if
int grid[60][80];
int colors[60][80];

BYTE GetDriverandReport() {
	BYTE i;
	BYTE rcode;
	BYTE device = 0xFF;
	BYTE tmpbyte;

	DEV_RECORD* tpl_ptr;
	printf("Reached USB_STATE_RUNNING (0x40)\n");
	for (i = 1; i < USB_NUMDEVICES; i++) {
		tpl_ptr = GetDevtable(i);
		if (tpl_ptr->epinfo != NULL) {
			printf("Device: %d", i);
			printf("%s \n", devclasses[tpl_ptr->devclass]);
			device = tpl_ptr->devclass;
		}
	}
	//Query rate and protocol
	rcode = XferGetIdle(addr, 0, hid_device.interface, 0, &tmpbyte);
	if (rcode) {   //error handling
		printf("GetIdle Error. Error code: ");
		printf("%x \n", rcode);
	} else {
		printf("Update rate: ");
		printf("%x \n", tmpbyte);
	}
	printf("Protocol: ");
	rcode = XferGetProto(addr, 0, hid_device.interface, &tmpbyte);
	if (rcode) {   //error handling
		printf("GetProto Error. Error code ");
		printf("%x \n", rcode);
	} else {
		printf("%d \n", tmpbyte);
	}
	return device;
}

void setLED(int LED) {
	IOWR_ALTERA_AVALON_PIO_DATA(LEDS_PIO_BASE,
			(IORD_ALTERA_AVALON_PIO_DATA(LEDS_PIO_BASE) | (0x001 << LED)));
}

void clearLED(int LED) {
	IOWR_ALTERA_AVALON_PIO_DATA(LEDS_PIO_BASE,
			(IORD_ALTERA_AVALON_PIO_DATA(LEDS_PIO_BASE) & ~(0x001 << LED)));

}

void printSignedHex0(signed char value) {
	BYTE tens = 0;
	BYTE ones = 0;
	WORD pio_val = IORD_ALTERA_AVALON_PIO_DATA(HEX_DIGITS_PIO_BASE);
	if (value < 0) {
		setLED(11);
		value = -value;
	} else {
		clearLED(11);
	}
	//handled hundreds
	if (value / 100)
		setLED(13);
	else
		clearLED(13);

	value = value % 100;
	tens = value / 10;
	ones = value % 10;

	pio_val &= 0x00FF;
	pio_val |= (tens << 12);
	pio_val |= (ones << 8);

	IOWR_ALTERA_AVALON_PIO_DATA(HEX_DIGITS_PIO_BASE, pio_val);
}

void printSignedHex1(signed char value) {
	BYTE tens = 0;
	BYTE ones = 0;
	DWORD pio_val = IORD_ALTERA_AVALON_PIO_DATA(HEX_DIGITS_PIO_BASE);
	if (value < 0) {
		setLED(10);
		value = -value;
	} else {
		clearLED(10);
	}
	//handled hundreds
	if (value / 100)
		setLED(12);
	else
		clearLED(12);

	value = value % 100;
	tens = value / 10;
	ones = value % 10;
	tens = value / 10;
	ones = value % 10;

	pio_val &= 0xFF00;
	pio_val |= (tens << 4);
	pio_val |= (ones << 0);

	IOWR_ALTERA_AVALON_PIO_DATA(HEX_DIGITS_PIO_BASE, pio_val);
}

void setKeycode(WORD keycode)
{
	IOWR_ALTERA_AVALON_PIO_DATA(0x00000160, keycode);
}

void keyInput(int x, int y, int index, int color) {
	int xChange = 0, yChange = 1, newIndex = index, newColor = color;
	BYTE rcode;
	BOOT_MOUSE_REPORT buf;		//USB mouse report
	BOOT_KBD_REPORT kbdbuf;

	BYTE runningdebugflag = 0;//flag to dump out a bunch of information when we first get to USB_STATE_RUNNING
	BYTE errorflag = 0; //flag once we get an error device so we don't keep dumping out state info
	BYTE device;
	WORD keycode;

	printf("initializing MAX3421E...\n");
	MAX3421E_init();
	printf("initializing USB...\n");
	USB_init();
	while (1) {
		//printf(".");
		MAX3421E_Task();
		USB_Task();
		//usleep (500000);
		if (GetUsbTaskState() == USB_STATE_RUNNING) {
			if (!runningdebugflag) {
				runningdebugflag = 1;
				setLED(9);
				device = GetDriverandReport();
			} else if (device == 1) {
				//run keyboard debug polling
				rcode = kbdPoll(&kbdbuf);
				if (rcode == hrNAK) {
					continue; //NAK means no new data
				} else if (rcode) {
					printf("Rcode: ");
					printf("%x \n", rcode);
					continue;
				}
				//printf("keycodes: ");
				for (int i = 0; i < 6; i++) {
					if (kbdbuf.keycode[i] != 0) {
						printf("%x - ", kbdbuf.keycode[i]);
						if (kbdbuf.keycode[i] == 26) { //W
							//printf("W %u , %u - %u ;",x,y, grid[y][x])
							yChange = -1;
							xChange = 0;
						} else if (kbdbuf.keycode[i] == 4) { //A
							//printf("A %u , %u - %u ;",x,y, grid[y][x]);
							xChange = -1;
						} else if (kbdbuf.keycode[i] == 22) { //S
							//printf("S %u , %u - %u ;",x,y, grid[y][x])
							yChange = 1;
							xChange = 0;
						} else if (kbdbuf.keycode[i] == 7) { //D
							//printf("D %u , %u - %u ;",x,y, grid[y][x])
							xChange = 1;
						}
					}
				}
				if (xChange == 1 && x >=75) {
					xChange*=-1;
				} else if (xChange == -1 && x <=5) {
					xChange*=1;
				} else if (yChange == -1 && y <=5) {
					yChange*=-1;
				} else if (yChange == 1 && y >=75) {
					yChange*=-1;
				} //VGA screen border
					switch(newIndex) {
					case 0  :
						if (grid[y+yChange][x] == 1){//if bottom pixel is going to be an edge
							setEdge(x,y);
							setEdge(x,y-1);
							setEdge(x,y-2);
							setEdge(x,y-3);//set as edge
						}
					break;

					if (grid[y+yChange][x] == 1){//if bottom pixel is going to be an edge
						case 1  :
							if ( grid[y-2][x+1] ==  1|| //if side block is an edge
								setEdge(x,y);
								setEdge(x,y-1);
								setEdge(x,y-2);
								setEdge(x,y-3);
								setEdge(x+1,y-3);//set as edge
							}
						break;
						case 2  :
							if (grid[y+1][x+1] ==  1|| //if side block is an edge
								grid[y+yChange][x] == 1){//if bottom pixel is going to be an edge
								setEdge(x,y);
								setEdge(x,y-1);
								setEdge(x,y-2);
								setEdge(x,y-3);
								setEdge(x+1,y);//set as edge
							}
						break;
						case 3  :
							if (grid[y+1][x+1] ==  1|| //if side block is an edge
								grid[y+1][x-1] ==  1|| //if side block is an edge
								grid[y+yChange][x] == 1){//if bottom pixel is going to be an edge
								setEdge(x,y);
								setEdge(x,y-1);
								setEdge(x+1,y);
								setEdge(x-1,y);//set as edge
							}
						break;
						case 4  :
							if (grid[y+1][x+1] ==  1|| //if side block is an edge
								grid[y+yChange][x] == 1){//if bottom pixel is going to be an edge
								setEdge(x,y);
								setEdge(x,y-1);
								setEdge(x+1,y);
								setEdge(x+1,y-1);//set as edge
							}
						break;
						case 5  :
							if (grid[y+1][x+1] ==  1 || //if side block is an edge
								grid[y][x-1] ==  1 || //if side block is an edge
								grid[y+yChange][x] == 1){//if bottom pixel is going to be an edge
								setEdge(x,y);
								setEdge(x,y-1);
								setEdge(x-1,y-1);
								setEdge(x+1,y);//set as edge
							}
						break;
						default :
							if (grid[y+1][x-1] ==  1|| //if side block is an edge
								grid[y][x+1] ==  1|| //if side block is an edge
								grid[y+yChange][x] == 1){//if bottom pixel is going to be an edge
								setEdge(x,y);
								setEdge(x,y-1);
								setEdge(x+1,y-1);
								setEdge(x-1,y);//set as edge
							}
					}
					yChange = 1; //stop
					xChange = 0;//stop
					newIndex = rand()%7; //new index
					newColor = rand()%15+1; //new color
					x = 40; //reset x
					if (newIndex <= 2) {
						y = 12;
					} else {
						y = 10;
					} //reset y
				}
				userControlledBlock(x, y, newIndex,0); //remove original location
				colors[y,x] = 0;
				x+=xChange; //update X
				y+=yChange; //update Y
				if (y > 10) {
					userControlledBlock(x, y, newIndex, newColor); //add to new location
					colors[y][x] = newColor;
				} //else
				xChange = 0; //reset to 0
				printf("(%u,%u) - %u ; \n",x,y, grid[y][x]);

			/*	for (int yCheck = 47; yCheck > 9; yCheck--) {

					if (checkRow(yCheck) == 1) {
						for (int xCheck = 10; xCheck < 68; xCheck++) {
							clearEdge(xCheck,yCheck);
						}
					}
				} /* //check edge

				setKeycode(kbdbuf.keycode[0]);
				printSignedHex0(kbdbuf.keycode[0]);
				printSignedHex1(kbdbuf.keycode[1]);
			}

			/*else if (device == 2) {
				rcode = mousePoll(&buf);
				if (rcode == hrNAK) {
					//NAK means no new data
					continue;
				} else if (rcode) {
					printf("Rcode: ");
					printf("%x \n", rcode);
					continue;
				}
				printf("X displacement: ");
				printf("%d ", (signed char) buf.Xdispl);
				printSignedHex0((signed char) buf.Xdispl);
				printf("Y displacement: ");
				printf("%d ", (signed char) buf.Ydispl);
				printSignedHex1((signed char) buf.Ydispl);
				printf("Buttons: ");
				printf("%x\n", buf.button);
				if (buf.button & 0x04)
					setLED(2);
				else
					clearLED(2);
				if (buf.button & 0x02)
					setLED(1);
				else
					clearLED(1);
				if (buf.button & 0x01)
					setLED(0);
				else
					clearLED(0);
			}*/
		} else if (GetUsbTaskState() == USB_STATE_ERROR) {
			if (!errorflag) {
				errorflag = 1;
				clearLED(9);
				printf("USB Error State\n");
				//print out string descriptor here
			}
		} else //not in USB running state
		{

			printf("USB task state: ");
			printf("%x\n", GetUsbTaskState());
			if (runningdebugflag) {	//previously running, reset USB hardware just to clear out any funky state, HS/FS etc
				runningdebugflag = 0;
				MAX3421E_init();
				USB_init();
			}
			errorflag = 0;
			clearLED(9);
		}
	}

}

void setEdge(int x, int y) {
	grid[y][x] = 1;
	//printf("set at: %u,%u; ",x,y);
}

void clearEdge(int x, int y) {
	grid[y][x] = 0;
	//printf("set at: %u,%u; ",x,y);
}


int checkRow(int y) {
	for (int xCheck = 10; xCheck < 68; xCheck++) {
		if (grid[y][xCheck] == 0) {
			return 1;
		}
	}
	return 0;
}

void paintScreen() {
	for (int row = 0; row < 60; row++) {
		for (int col = 0; col < 80; col++) {
				VGADrawColorBox(row, col, colors[row][col], 0);
		}
	}
}

void updateBlock() {
	
}
