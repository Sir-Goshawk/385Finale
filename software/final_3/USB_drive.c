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
#include <time.h>


typedef struct
{
    int width;
    int height;
    char data[5][5];
    int color;
} TetrisBlock;

// clang-format off
const TetrisBlock tetrisBlocks[] =
{
    {
        2, // width
        2, // height
        {"XX",
         "XX"},
    },
    {
        3, // width
        2, // height
        {" X ",
         "XXX"},
    },
    {
        4, // width
        1, // height
        {"XXXX"},
    },
    {
        2, // width
        3, // height
        {"XX",
         "X ",
         "X "},
    },
    {
        2, // width
        3, // height
        {"XX",
         " X",
         " X"},
    },
    {
        3, // width
        2, // height
        {"XX ",
         " XX"},
    },
	{
			3,
			2,
			{" XX",
			 "XX "}
	}
};
// clang-format on


typedef struct
{
    int** board;

    int boardWidth;
    int boardHeight;

    int dead;
    int completedLines;

    TetrisBlock activeBlock;

    int activeBlockX;
    int activeBlockY;
    int pause;
    int gameX;
    int gameY;
    int boardColor;
} TetrisGameState;

int boardColor;
int nextBlock, nextColor;
const int tetrisBlockCount = sizeof(tetrisBlocks) / sizeof(TetrisBlock);
int players = 0;
int arrowPos = 25;
alt_u8* timer = 0x00000040;
alt_u8* reset = 0x00000030;


void TetrisInitialize(TetrisGameState* givenState, int width, int height, int givenX, int givenY) {
    int x, y;
    givenState->completedLines = 0;
    givenState->dead = 0;
    givenState->boardWidth = (width);
    givenState->boardHeight = (height+4);
    givenState->gameX = givenX;
    givenState->gameY = givenY;
    givenState->board = (int**)malloc(sizeof(int*) * width);

    for (x = 0; x < givenState->boardWidth; x++) {
        givenState->board[x] = (int*)malloc(sizeof(int) * givenState->boardHeight);
        for (y = 0; y < givenState->boardHeight; y++) {
            givenState->board[x][y] = givenState->boardColor;
        }
    }
    TetrisPrintBoard(givenState);

    givenState->pause = 0;
}

void TetrisCleanup(TetrisGameState* givenState) {
    for (int x = 0; x < givenState->boardWidth; x++) {
    	for (int y = 0; y < givenState->boardHeight; y++) {
    		givenState->board[x][y] = givenState->boardColor;
    	}
        //free(givenState->board[x]);
    }
    //free(givenState->board);
}


int TetrisCheckCollision(TetrisGameState* givenState) {
    int x, y, X, Y;

    TetrisBlock activeBlock = givenState->activeBlock;

    for (x = 0; x < activeBlock.width; x++) {
        for (y = 0; y < activeBlock.height; y++) {
            X = givenState->activeBlockX + x;
            Y = givenState->activeBlockY + y;

            if (X < 0 || X >= givenState->boardWidth) return 1;

            if (activeBlock.data[y][x] != ' ') {
                if ((Y >= givenState->boardHeight) ||
                    (X >= 0 && X < givenState->boardWidth && Y >= 3 &&
						givenState->board[X][Y] != givenState->boardColor)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

void TetrisCreateBlock(TetrisGameState* givenState) {
    givenState->activeBlock = tetrisBlocks[nextBlock];

    givenState->activeBlockX =
        (givenState->boardWidth / 2) - (givenState->activeBlock.width / 2);

    givenState->activeBlockY = 4-givenState->activeBlock.height;

    givenState->activeBlock.color = nextColor; //current block's color
    nextBlock = rand() % tetrisBlockCount; //next block's color
  	//nextBlock = 0;
    int color = (rand() % 15)+1;
    if (color == givenState->boardColor || color == nextColor) color = (rand() % 15)+1;
    nextColor = color; //next color
  	for (int i = 0; i < rand() % 3; i++) {
  		TetrisRotateBlock(givenState); //roate block
  	}
  	TetrisBlock previewBlock = tetrisBlocks[nextBlock]; //preview next block
	for (int x = 0; x < 9; x++) {
		for (int y = 0; y < 9; y++) {
			if ((x < previewBlock.width &&  y < previewBlock.height) && previewBlock.data[y/2][x/2] != ' ') 	VGADrawColorBox(x+givenState->gameX-10, y+givenState->gameY, color);
			else VGADrawColorBox(x+givenState->gameX-10, y+givenState->gameY, givenState->boardColor);
  		}
  	}

    if (TetrisCheckCollision(givenState)) {
        givenState->dead = 1;
		printf("[%d,%d] end game",givenState->gameX,givenState->gameY);
		VGAwriteText(givenState->gameX-(strlen("GAME OVER")),(givenState->gameY+4+(givenState->boardHeight)),2,0,"GAME OVER");
    }
}

void TetrisPrintBlock(TetrisGameState* givenState) {
    TetrisBlock activeBlock = givenState->activeBlock;

    int x, y;

    for (x = 0; x < activeBlock.width; x++) {
        for (y = 0; y < activeBlock.height; y++) {
            if (activeBlock.data[y][x] != ' ') {
				givenState->board[givenState->activeBlockX + x][givenState->activeBlockY + y] = activeBlock.color;
            }
        }
    }
}

void TetrisRotateBlock(TetrisGameState* givenState) {
    TetrisBlock b = givenState->activeBlock;
    TetrisBlock s = b;

    int x, y;

    b.width = s.height;
    b.height = s.width;

    for (x = 0; x < s.width; x++) {
        for (y = 0; y < s.height; y++) {
            b.data[x][y] = s.data[s.height - y - 1][x];
        }
    }

    x = givenState->activeBlockX;
    y = givenState->activeBlockY;

    givenState->activeBlockX -= (b.width - s.width) / 2;
    givenState->activeBlockY -= (b.height - s.height) / 2;
    givenState->activeBlock = b;

    if (TetrisCheckCollision(givenState)) {
        givenState->activeBlock = s;
        givenState->activeBlockX = x;
        givenState->activeBlockY = y;
    }
}

void TetrisFallBlocks(TetrisGameState* givenState) {
	++givenState->activeBlockY;

	if (TetrisCheckCollision(givenState)) {
		--givenState->activeBlockY;
		TetrisPrintBlock(givenState);
		TetrisCreateBlock(givenState);
	}
}

void TetrisClearLine(TetrisGameState* givenState, int l) {
    int x, y;

    for (y = l; y > 4; y--) {
        for (x = 0; x < givenState->boardWidth; x++) {
            givenState->board[x][y] = givenState->board[x][y - 1];
        }
    }

    for (x = 0; x < givenState->boardWidth; x++) {
        givenState->board[x][0] = givenState->boardColor;
    }
}

void TetrisCheckLineComplete(TetrisGameState* givenState) {
    int x, y, l;

    for (y = givenState->boardHeight - 1; y >= 0; y--) {
        l = 1;

        for (x = 0; x < givenState->boardWidth && l; x++) {
            if (givenState->board[x][y] == givenState->boardColor) {
                l = 0;
            }
        }

        if (l) {
            ++(givenState->completedLines);
            TetrisClearLine(givenState, y);
            int holder = (givenState->completedLines);
            holder = holder*100;
            printf("%d line(s) completed at %d\n", givenState->completedLines, holder);
            char showScore;
        	sprintf(showScore, "%d", holder);
            VGAwriteText(givenState->gameX-10, givenState->gameY+12, 14, givenState->boardColor, showScore);
            y++;
        }
    }
}

void TetrisInputLeft(TetrisGameState* givenState) {
    --givenState->activeBlockX;
    if (TetrisCheckCollision(givenState)) ++givenState->activeBlockX;
}

void TetrisInputRight(TetrisGameState* givenState) {
    ++givenState->activeBlockX;
    if (TetrisCheckCollision(givenState)) --givenState->activeBlockX;
}

void TetrisPrintBoard(TetrisGameState* givenState) {
    int x, y;
    //printf("drawing start (%d, %d)", givenState->gameX, givenState->gameY);
	if (givenState->pause) {
		VGAwriteText(givenState->gameX-(strlen("PAUSE GAME")),(givenState->gameY+(givenState->boardHeight)),0,2,"PAUSE GAME");
	} else {
		VGAwriteText(givenState->gameX-(strlen("PAUSE GAME")),(givenState->gameY+(givenState->boardHeight)),0,0,"PAUSE GAME");
		for (y = 4; y < (givenState->boardHeight)*2; y++) {
			for (x = 0; x < (givenState->boardWidth)*2; x++) {
				if (x/2 >= givenState->activeBlockX &&                              //
					y/2 >= givenState->activeBlockY &&                              //
					x/2 < (givenState->activeBlockX + givenState->activeBlock.width) &&  //
					y/2 < (givenState->activeBlockY + givenState->activeBlock.height) && //
					givenState->activeBlock.data[y/2 - givenState->activeBlockY]
										   [x/2 - givenState->activeBlockX] != ' ') {
					VGADrawColorBox(givenState->gameX+x,givenState->gameY-4+y, givenState->activeBlock.color);
				} else {
					VGADrawColorBox(givenState->gameX+x,givenState->gameY-4+y,givenState->board[x/2][y/2]);}
			}
		}
		/*for (y = 4; y < (givenState->boardHeight); y++) {
			for (x = 0; x < (givenState->boardWidth); x++) {
				if (x >= givenState->activeBlockX &&                              //
					y >= givenState->activeBlockY &&                              //
					x < (givenState->activeBlockX + givenState->activeBlock.width) &&  //
					y < (givenState->activeBlockY + givenState->activeBlock.height) && //
					givenState->activeBlock.data[y - givenState->activeBlockY]
										   [x - givenState->activeBlockX] != ' ') {
					VGADrawColorBox(givenState->gameX+x,givenState->gameY-4+y, givenState->activeBlock.color);
				} else {
					VGADrawColorBox(givenState->gameX+x,givenState->gameY-4+y,givenState->board[x][y]);}
			}
		}*/
	}
}

void TetrisShowScore(int x, int y) {
    //char showScore;
	//sprintf(showScore, "%d", score);
}

void TetrisPause(TetrisGameState* givenState) {
	givenState->pause = !givenState->pause;
}

extern HID_DEVICE hid_device;

static BYTE addr = 1; 				//hard-wired USB address
const char* const devclasses[] = { " Uninitialized", " HID Keyboard", " HID Mouse", " Mass storage" };


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

void setKeycode(WORD keycode) {
	IOWR_ALTERA_AVALON_PIO_DATA(0x00000160, keycode);
}
TetrisGameState TetrisPopulate(int x, int y, int boardcolor, int width, int height) {
	TetrisGameState state; //new game
	state.boardColor = boardcolor; //set board color
	//nextBlock = 0;
	nextBlock = rand() % tetrisBlockCount; //intize first block
	int color = (rand() % 15)+1;
	if (color == boardcolor || color == nextColor) color = (rand() % 15)+1;
	nextColor = color; //initize first color
	TetrisInitialize(&state, width, height, x, y);
	TetrisCreateBlock(&state);
	VGAwriteText(x+width-(strlen("TETRIS")/2),y-2,2,0,"TETRIS");
    for (int i = 0; i < 9; i++) {
    	for (int j = 0; j < 6; j++) {
    		VGADrawColorBox(x-10+i,y+10+j,boardcolor);
    	}
    }
    VGAwriteText(x-10, y+10, 14, boardcolor, "SCORE:");
    VGAwriteText(x-10, y+12, 14, boardcolor, "0");
    return state;
}

void gameMenu() {
	for (int i = 0; i < 12; i++) {
		for (int j = 0; j < 4; j++) {
    		VGADrawColorBox(33+i,24+j,1);
		}
	}
    VGAwriteText(34, 25, 14, 1, "1 PLAYER");
    VGAwriteText(34, 27, 14, 1, "2 PLAYERS");
}

void moveMenu() {
	if (arrowPos == 25) VGAwriteText(34, arrowPos, 14, 1, ">");
	else {
		VGAwriteText(34, 27, 14, 1, ">");
		arrowPos = 27;
	}
}

void clearMenu() {
	for (int i = 0; i < 10; i++) {
		for (int j = 0; j < 4; j++) {
    		VGADrawColorBox(32+i,24+j,0);
		}
	}
}

void keyInput(int x, int y, int boardcolor, int width, int height) {
	srand(time(NULL));
	gameMenu();
	TetrisGameState state = TetrisPopulate(x, y, boardcolor,width, height);


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

	long counter;
	while (1) {
		srand(time(NULL));
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
			  if(!state.dead) {

				  if (timer != 0 && !state.pause) {
						TetrisFallBlocks(&state);
						//printf("%ul \n",time(NULL));
						reset = 1;
						reset = 0;
				  }
					TetrisPrintBoard(&state);
					TetrisCheckLineComplete(&state);
				}
				//printf("a");
				for (int i = 0; i < 6; i++) {
					if (kbdbuf.keycode[i] != 0) {
						//printf("%x - ", kbdbuf.keycode[i]);
						if (kbdbuf.keycode[i] == 26 || kbdbuf.keycode[i] == 82) { //W
							//printf("W - ");
							TetrisRotateBlock(&state);
						} else if (kbdbuf.keycode[i] == 4 || kbdbuf.keycode[i] == 80) { //A
							//printf("A - ");
							TetrisInputLeft(&state);
						} else if (kbdbuf.keycode[i] == 22 || kbdbuf.keycode[i] == 81) { //S
							//printf("S - ");
							TetrisFallBlocks(&state);
						} else if (kbdbuf.keycode[i] == 7 || kbdbuf.keycode[i] == 79) { //D
							//printf("D - ");
							TetrisInputRight(&state);
						} else if (kbdbuf.keycode[i] == 41) { //esc
							TetrisPause(&state);
							//printf("pause game \n");
						} else if (kbdbuf.keycode[i] == 40) { //enter
							if (state.dead) {
								TetrisCleanup(&state);
								state.dead = 0;
								VGAwriteText(x-(strlen("GAME OVER")),(y+8+(height)),0,0,"GAME OVER");
								printf("(%d,%d) \n",x,y );
							}
							if (players == 0) {
								if (arrowPos == 25) {
									//TetrisGameState state = TetrisPopulate(x, y, boardcolor,width, height);
									clearMenu();
								}
							}
						}
					}
				}
				setKeycode(kbdbuf.keycode[0]);
				printSignedHex0(kbdbuf.keycode[0]);
				printSignedHex1(kbdbuf.keycode[1]);
			}
		} else if (GetUsbTaskState() == USB_STATE_ERROR) {
			if (!errorflag) {
				errorflag = 1;
				clearLED(9);
				printf("USB Error State\n");
				//print out string descriptor here
			}
		} else { //not in USB running state

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
	printf("ends");
}
