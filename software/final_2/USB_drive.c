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
		 "XX "
		}
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
    //int pause;
} TetrisGameState;

int boardColor = 1;
int nextBlock = 2, nextColor = 9;
const int tetrisBlockCount = sizeof(tetrisBlocks) / sizeof(TetrisBlock);
int colorValues[80][60];
int level = 2;
int score;

void TetrisInitialize(TetrisGameState* state, int width, int height) {
    int x, y;

    state->completedLines = 0;
    state->dead = 0;
    state->boardWidth = width;
    //state->boardHeight = height+4;
    state->boardHeight = height;
    state->board = (int**)malloc(sizeof(int*) * width);

    for (x = 0; x < width; x++) {
        state->board[x] = (int*)malloc(sizeof(int) * height);
        for (y = 0; y < height; y++) {
            //state->board[x][y] = ' ';
            state->board[x][y] = boardColor;
        }
    }

    //state->pause = 0;
}

void TetrisCleanup(TetrisGameState* state) {
    int x;
    for (x = 0; x < state->boardWidth; x++) {
        free(state->board[x]);
    }
    free(state->board);
}

/*void TetrisPrintBoard(TetrisGameState* state)
{
    int x, y;

    for (x = 0; x < 30; x++)
    {
        printf("\n");
    }

    printf("%d line(s) completed\n\n", state->completedLines);

    for (y = 0; y < state->boardHeight; y++)
    {
        printf("|");

        for (x = 0; x < state->boardWidth; x++)
        {
            if (x >= state->activeBlockX &&                              //
                y >= state->activeBlockY &&                              //
                x < (state->activeBlockX + state->activeBlock.width) &&  //
                y < (state->activeBlockY + state->activeBlock.height) && //
                state->activeBlock.data[y - state->activeBlockY]
                                       [x - state->activeBlockX] != ' ')
            {
                printf("X");
            }
            else
            {
                printf("%c", state->board[x][y]);
            }
        }

        printf("|\n");
    }

    for (x = 0; x < state->boardWidth + 2; x++)
    {
        printf("-");
    }

    printf("\n");
}*/

int TetrisCheckCollision(TetrisGameState* state) {
    int x, y, X, Y;

    TetrisBlock activeBlock = state->activeBlock;

    for (x = 0; x < activeBlock.width; x++) {
        for (y = 0; y < activeBlock.height; y++) {
            X = state->activeBlockX + x;
            Y = state->activeBlockY + y;

            if (X < 0 || X >= state->boardWidth) return 1;

            if (activeBlock.data[y][x] != ' ') {
                if ((Y >= state->boardHeight) ||
                    //(X >= 0 && X < state->boardWidth && Y >= 3 &&
                    (X >= 0 && X < state->boardWidth && Y >= 0 &&
                    	//state->board[X][Y] != ' '))
						state->board[X][Y] != boardColor)) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

void TetrisCreateBlock(TetrisGameState* state) {
    state->activeBlock = tetrisBlocks[nextBlock];

    state->activeBlockX =
        (state->boardWidth / 2) - (state->activeBlock.width / 2);

    //state->activeBlockY = 4-state->activeBlock.height;
    state->activeBlockY = 3;

    state->activeBlock.color = nextColor; //current block's color

    //nextBlock = rand() % tetrisBlockCount; //next block's color
  	int color = (rand() % 15)+1;
    if (color == boardColor || color == nextColor) color = (rand() % 15)+1;
    //nextColor = color; //next color
  	for (int i = 0; i < rand() % 3; i++) {
  		TetrisRotateBlock(state); //roate block
  	}
  	TetrisBlock previewBlock = tetrisBlocks[nextBlock]; //preview next block
  	for (int x = 0; x < 5; x++) {
  		for (int y = 0; y < 5; y++) {
			if ((x < previewBlock.width &&  y < previewBlock.height) && previewBlock.data[y][x] != ' ') 	VGADrawColorBox(x+14+2-previewBlock.width/2, y+10, color);
			else VGADrawColorBox(x+14, y+10, boardColor);
  		}
  	}

    if (TetrisCheckCollision(state)) {
        state->dead = 1;
		printf("end game");
		VGAwriteText(25-(strlen("GAME OVER")/2),(10+24/2),0,2,"GAME OVER");
    }
}

void TetrisPrintBlock(TetrisGameState* state) {
    TetrisBlock activeBlock = state->activeBlock;

    int x, y;

    for (x = 0; x < activeBlock.width; x++) {
        for (y = 0; y < activeBlock.height; y++) {
            if (activeBlock.data[y][x] != ' ') {
				state->board[state->activeBlockX + x][state->activeBlockY + y] =
					//b.data[y][x];
					activeBlock.color;
            }
        }
    }
}

void TetrisRotateBlock(TetrisGameState* state) {
    TetrisBlock b = state->activeBlock;
    TetrisBlock s = b;

    int x, y;

    b.width = s.height;
    b.height = s.width;

    for (x = 0; x < s.width; x++) {
        for (y = 0; y < s.height; y++) {
            b.data[x][y] = s.data[s.height - y - 1][x];
        }
    }

    x = state->activeBlockX;
    y = state->activeBlockY;

    state->activeBlockX -= (b.width - s.width) / 2;
    state->activeBlockY -= (b.height - s.height) / 2;
    state->activeBlock = b;

    if (TetrisCheckCollision(state)) {
        state->activeBlock = s;
        state->activeBlockX = x;
        state->activeBlockY = y;
    }
}

void TetrisFallBlocks(TetrisGameState* state) {
	++state->activeBlockY;

	if (TetrisCheckCollision(state)) {
		--state->activeBlockY;
		TetrisPrintBlock(state);
		TetrisCreateBlock(state);
	}
}

void TetrisClearLine(TetrisGameState* state, int l) {
    int x, y;

    //for (y = l; y > 4; --y) {
    for (y = l; y > 0; y--) {
        for (x = 0; x < state->boardWidth; x++) {
            state->board[x][y] = state->board[x][y - 1];
        }
    }

    for (x = 0; x < state->boardWidth; x++) {
        //state->board[x][0] = ' ';
        state->board[x][0] = boardColor;
    }
}

void TetrisCheckLineComplete(TetrisGameState* state) {
    int x, y, l;

    for (y = state->boardHeight - 1; y >= 0; y--) {
        l = 1;

        for (x = 0; x < state->boardWidth && l; x++) {
            //if (state->board[x][y] == ' ')
            if (state->board[x][y] == boardColor) {
                l = 0;
            }
        }

        if (l) {
            ++(state->completedLines);
            TetrisClearLine(state, y);
            score+=level/2*100;
            printf("%d line(s) completed at %d score %d\n", state->completedLines, y, score);
            y++;
        }
    }
}

void TetrisInputLeft(TetrisGameState* state) {
    --state->activeBlockX;

    if (TetrisCheckCollision(state)) ++state->activeBlockX;
}

void TetrisInputRight(TetrisGameState* state) {
    ++state->activeBlockX;

    if (TetrisCheckCollision(state)) --state->activeBlockX;
}

void TetrisMain(int boardWidth, int boardHeight) {
    TetrisGameState state;

    TetrisInitialize(&state, boardWidth, boardHeight);
    TetrisCreateBlock(&state);

    while (!state.dead)
    {
        TetrisPrintBoard(&state);
        TetrisFallBlocks(&state);
        TetrisCheckLineComplete(&state);

        char cmd;
        scanf("%c", &cmd);

        switch (cmd)
        {
            case 'a':
            {
                TetrisInputLeft(&state);
                break;
            }

            case 'd':
            {
                TetrisInputRight(&state);
                break;
            }

            case 's':
            {
                TetrisFallBlocks(&state);
                break;
            }

            case 'w':
            {
                TetrisRotateBlock(&state);
                break;
            }
        }
    }

    TetrisPrintBoard(&state);
    printf("GAME OVER\n");

    TetrisCleanup(&state);
}

void TetrisPrintBoard(TetrisGameState* state, int givenX, int givenY) {
    int x, y;

    //for (y = 4; y < state->boardHeight; y++) {
    for (y = 0; y < state->boardHeight; y++) {
        for (x = 0; x < state->boardWidth; x++) {
            if (x >= state->activeBlockX &&                              //
                y >= state->activeBlockY &&                              //
                x < (state->activeBlockX + state->activeBlock.width) &&  //
                y < (state->activeBlockY + state->activeBlock.height) && //
                state->activeBlock.data[y - state->activeBlockY]
                                       [x - state->activeBlockX] != ' ') {
            	VGADrawColorBox(givenX+x,givenY+y, state->activeBlock.color);
            } else {
                if (board[givenX+x][givenY+y] != boardColor) VGADrawColorBox(givenX+x,givenY+y,state->board[x][y]);
                else VGADrawColorBox(givenX+x,givenY+y,state->board[x][y]);
                if (state->board[x][y] != 1) printf("(%d,%d) %d",x,y,state->board[x][y]);
            }
        }
    }
}

void paintScreen() {
	for (int x = 0; x < 80; x++) {
		for (int y = 0; y < 60; y++) {
        	VGADrawColorBox(x,y, colorValues[x][y]);
		}
	}
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


void keyInput(int x, int y, int boardcolor, int width, int height) {
	TetrisGameState state; //new game
	boardColor = boardcolor; //set board color
	//nextBlock = rand() % tetrisBlockCount; //intize first block
	int color = (rand() % 15)+1;
  if (color == boardColor || color == nextColor) color = (rand() % 15)+1;
  //nextColor = color; //initize first color
  TetrisInitialize(&state, width, height);
  TetrisCreateBlock(&state);

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
	int count = 0;
	while (/*1) {*/!state.dead) {

  		count++;
  		if (count  == level) {
  			//printf("cout %d", count);
  			count = 0;
  			TetrisFallBlocks(&state);
  		}
  		TetrisPrintBoard(&state, x, y);
  		TetrisCheckLineComplete(&state);

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

      //if(!state.dead && !state.pause) {
          //TetrisPrintBoard(&state);
      		//paintScreen();
  				//printf("keycodes: ");
  				for (int i = 0; i < 6; i++) {
  					if (kbdbuf.keycode[i] != 0) {
  						//printf("%x - ", kbdbuf.keycode[i]);
  						if (kbdbuf.keycode[i] == 26 || kbdbuf.keycode[i] == 82) { //W
  							printf("W - ");
  			                TetrisRotateBlock(&state);
  						} else if (kbdbuf.keycode[i] == 4 || kbdbuf.keycode[i] == 80) { //A
  							printf("A - ");
  			                TetrisInputLeft(&state);
  						} else if (kbdbuf.keycode[i] == 22 || kbdbuf.keycode[i] == 81) { //S
  							printf("S - ");
  			                TetrisFallBlocks(&state);
  						} else if (kbdbuf.keycode[i] == 7 || kbdbuf.keycode[i] == 79) { //D
  							printf("D - ");
  			                TetrisInputRight(&state);
  						} else if (kbdbuf.keycode[i] == 41) { //esc
  							//state.pause = 1;
  							printf("pause game");
  							VGAwriteText(x+x/2-(strlen("PAUSE GAME")/2),(y+24/2),2,0,"PAUSE GAME");
  						} else if (kbdbuf.keycode[i] == 40) { //enter
  						  //state.pause = !state.pause;
  						  printf("enter");
  						}
  					}
  				}
        //}


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
}
