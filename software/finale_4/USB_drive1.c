#include <stdio.h>
#include <assert.h>
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


typedef enum {
    e_TETRIS_BLOCK_TYPE_O = 0,
    e_TETRIS_BLOCK_TYPE_T = 1,
    e_TETRIS_BLOCK_TYPE_I = 2,
    e_TETRIS_BLOCK_TYPE_J = 3,
    e_TETRIS_BLOCK_TYPE_L = 4,
    e_TETRIS_BLOCK_TYPE_S = 5,
    e_TETRIS_BLOCK_TYPE_S_REVERSE = 6,

    e_TETRIS_BLOCK_TYPE_COUNT
} TetrisBlockType;

typedef enum {
    e_TETRIS_DIFFICULTY_LEVEL_EASY = 0,
    e_TETRIS_DIFFICULTY_LEVEL_MEDIUM = 1,
    e_TETRIS_DIFFICULTY_LEVEL_HARD = 2,

    e_TETRIS_DIFFICULTY_LEVEL_COUNT
} TetrisDifficultyLevel;

typedef struct {
    TetrisBlockType blockType;
    int width;
    int height;
    char data[5][5];
    int color;
} TetrisBlock;

typedef struct {
    int totalChance;
    int cumulativeChances[e_TETRIS_BLOCK_TYPE_COUNT];
} TetrisBlockChanceTable;

typedef struct {
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
    int nextBlock;
    int nextColor;
    TetrisDifficultyLevel difficulty;
    TetrisBlockChanceTable blockChanceTables[e_TETRIS_DIFFICULTY_LEVEL_COUNT];
} TetrisGameState;


const TetrisBlock tetrisBlocks[e_TETRIS_BLOCK_TYPE_COUNT] =
{
    {
        e_TETRIS_BLOCK_TYPE_O,
        2, // width
        2, // height
        {"XX",
         "XX"},
    },

    {
        e_TETRIS_BLOCK_TYPE_T,
        3, // width
        2, // height
        {" X ",
         "XXX"},
    },

    {
        e_TETRIS_BLOCK_TYPE_I,
        4, // width
        1, // height
        {"XXXX"},
    },

    {
        e_TETRIS_BLOCK_TYPE_J,
        2, // width
        3, // height
        {"XX",
         "X ",
         "X "},
    },

    {
        e_TETRIS_BLOCK_TYPE_L,
        2, // width
        3, // height
        {"XX",
         " X",
         " X"},
    },

    {
        e_TETRIS_BLOCK_TYPE_S,
        3, // width
        2, // height
        {"XX ",
         " XX"},
    },

    {
        e_TETRIS_BLOCK_TYPE_S_REVERSE,
        3, // width
        2, // height
        {" XX",
         "XX "},
    }
};

int boardColor;
int nextBlock, nextColor;
const int tetrisBlockCount = sizeof(tetrisBlocks) / sizeof(TetrisBlock);
int players = 0;
int arrowPos = 25;
TetrisGameState state1, state2;
int difficultyPos1 = 0, difficultyPos2 = 0, difficulty1 = 0; difficulty2 = 0, run1=0, run2=0;
alt_u8* timer = 0x00000040;
alt_u8* reset = 0x00000030;


void tetrisInitializeChanceTable(TetrisBlockChanceTable* table, int* chances){
    assert(e_TETRIS_BLOCK_TYPE_O == 0);
    assert(e_TETRIS_BLOCK_TYPE_T == 1);
    assert(e_TETRIS_BLOCK_TYPE_S_REVERSE == 6);

    table->totalChance = chances[0];
    table->cumulativeChances[e_TETRIS_BLOCK_TYPE_O] = chances[0];

    for (int i = e_TETRIS_BLOCK_TYPE_T; i < e_TETRIS_BLOCK_TYPE_COUNT; ++i)
    {
        table->totalChance += chances[i];
        table->cumulativeChances[i] =
            table->cumulativeChances[i - 1] + chances[i];
    }

    assert(table->cumulativeChances[e_TETRIS_BLOCK_TYPE_S_REVERSE] ==
           table->totalChance);
}

int tetrisGetRandomTetrisBlock(TetrisGameState* givenState){
    assert(e_TETRIS_BLOCK_TYPE_O == 0);

    TetrisBlockChanceTable* chanceTable =
        &givenState->blockChanceTables[givenState->difficulty];

    int rndValue = rand() % chanceTable->totalChance;

    for (int i = e_TETRIS_BLOCK_TYPE_O; i < e_TETRIS_BLOCK_TYPE_COUNT; ++i)
    {
        if (chanceTable->cumulativeChances[i] >= rndValue)
        {
            return i;
        }
    }

	printf("%d, %d",rndValue, chanceTable->totalChance);
    assert(0);
    return e_TETRIS_BLOCK_TYPE_O;
}

void tetrisInitializeBlockChanceTables(TetrisGameState* givenState) {
    {
        int chances[e_TETRIS_BLOCK_TYPE_COUNT];

        chances[e_TETRIS_BLOCK_TYPE_O] = 10;
        chances[e_TETRIS_BLOCK_TYPE_T] = 10;
        chances[e_TETRIS_BLOCK_TYPE_I] = 10;
        chances[e_TETRIS_BLOCK_TYPE_J] = 10;
        chances[e_TETRIS_BLOCK_TYPE_L] = 10;
        chances[e_TETRIS_BLOCK_TYPE_S] = 10;
        chances[e_TETRIS_BLOCK_TYPE_S_REVERSE] = 10;

        tetrisInitializeChanceTable(
            &givenState->blockChanceTables[e_TETRIS_DIFFICULTY_LEVEL_EASY], chances);
    }

    {
        int chances[e_TETRIS_BLOCK_TYPE_COUNT];

        chances[e_TETRIS_BLOCK_TYPE_O] = 20;
        chances[e_TETRIS_BLOCK_TYPE_T] = 15;
        chances[e_TETRIS_BLOCK_TYPE_I] = 10;
        chances[e_TETRIS_BLOCK_TYPE_J] = 20;
        chances[e_TETRIS_BLOCK_TYPE_L] = 20;
        chances[e_TETRIS_BLOCK_TYPE_S] = 22;
        chances[e_TETRIS_BLOCK_TYPE_S_REVERSE] = 22;

        tetrisInitializeChanceTable(
            &givenState->blockChanceTables[e_TETRIS_DIFFICULTY_LEVEL_MEDIUM],
            chances);
    }

    {
        int chances[e_TETRIS_BLOCK_TYPE_COUNT];

        chances[e_TETRIS_BLOCK_TYPE_O] = 30;
        chances[e_TETRIS_BLOCK_TYPE_T] = 25;
        chances[e_TETRIS_BLOCK_TYPE_I] = 20;
        chances[e_TETRIS_BLOCK_TYPE_J] = 35;
        chances[e_TETRIS_BLOCK_TYPE_L] = 35;
        chances[e_TETRIS_BLOCK_TYPE_S] = 40;
        chances[e_TETRIS_BLOCK_TYPE_S_REVERSE] = 40;

        tetrisInitializeChanceTable(
            &givenState->blockChanceTables[e_TETRIS_DIFFICULTY_LEVEL_HARD], chances);
    }
}

void tetrisInitialize(TetrisGameState* givenState, int width, int height, int givenX, int givenY, int difficult) {
    givenState->completedLines = 0;
    givenState->dead = 0;
    givenState->boardWidth = (width);
    givenState->boardHeight = (height+4);
    givenState->gameX = givenX;
    givenState->gameY = givenY;
    givenState->board = (int**)malloc(sizeof(int*) * width);
    if (difficult == 3) givenState->difficulty = e_TETRIS_DIFFICULTY_LEVEL_HARD;
    if (difficult == 2) givenState->difficulty = e_TETRIS_DIFFICULTY_LEVEL_MEDIUM;
    else givenState->difficulty = e_TETRIS_DIFFICULTY_LEVEL_EASY;
    tetrisInitializeBlockChanceTables(givenState);


    for (int x = 0; x < givenState->boardWidth; x++) {
        givenState->board[x] = (int*)malloc(sizeof(int) * givenState->boardHeight);
        for (int y = 0; y < givenState->boardHeight; y++) {
            givenState->board[x][y] = givenState->boardColor;
        }
    }
    tetrisPrintBoard(givenState);

    givenState->pause = 0;
}

void tetrisCleanup(TetrisGameState* givenState) {
    for (int x = 0; x < givenState->boardWidth; x++) {
    	for (int y = 0; y < givenState->boardHeight; y++) {
    		givenState->board[x][y] = givenState->boardColor;
    	}
        //free(givenState->board[x]);
    }
    //free(givenState->board);
}


int tetrisCheckCollision(TetrisGameState* givenState) {
    int X, Y;

    tetrisBlock activeBlock = givenState->activeBlock;

    for (int x = 0; x < activeBlock.width; x++) {
        for (int y = 0; y < activeBlock.height; y++) {
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

void tetrisCreateBlock(TetrisGameState* givenState) {
    givenState->activeBlock = tetrisBlocks[tetrisGetRandomTetrisBlock(givenState)];

    givenState->activeBlockX = (givenState->boardWidth / 2) - (givenState->activeBlock.width / 2);

    givenState->activeBlockY = 4-givenState->activeBlock.height;

    givenState->activeBlock.color = givenState->nextColor; //current block's color
    givenState->nextBlock = tetrisGetRandomTetrisBlock(givenState); //next block's color
    int color = (rand() % 15)+1;
    if (color == givenState->boardColor || color == nextColor) color = (rand() % 15)+1;
    givenState->nextColor = color; //next color
  	for (int i = 0; i < rand() % 3; i++) {
  		tetrisRotateBlock(givenState); //roate block
  	}
  	TetrisBlock previewBlock = tetrisBlocks[givenState->nextBlock]; //preview next block
	for (int x = 0; x < 9; x++) {
		for (int y = 0; y < 9; y++) {
			if ((x < previewBlock.width*2 &&  y < previewBlock.height*2) && previewBlock.data[y/2][x/2] != ' ') VGADrawColorBox(x+givenState->gameX-10, y+givenState->gameY, givenState->nextColor);
			else VGADrawColorBox(x+givenState->gameX-10, y+givenState->gameY, givenState->boardColor);
  		}
  	}

    if (tetrisCheckCollision(givenState)) {
        givenState->dead = 1;
		printf("[%d,%d] end game",givenState->gameX,givenState->gameY);
		VGAwriteText(givenState->gameX-(strlen("GAME OVER")),(givenState->gameY+4+(givenState->boardHeight)),2,0,"GAME OVER");
    }
}

void tetrisPrintBlock(TetrisGameState* givenState) {
    TetrisBlock activeBlock = givenState->activeBlock;

    for (int x = 0; x < activeBlock.width; x++) {
        for (int y = 0; y < activeBlock.height; y++) {
            if (activeBlock.data[y][x] != ' ') {
              givenState->board[givenState->activeBlockX + x][givenState->activeBlockY + y] = activeBlock.color;
            }
        }
    }
}

void tetrisRotateBlock(TetrisGameState* givenState) {
    TetrisBlock oldBlock = givenState->activeBlock;
    TetrisBlock newBlock = oldBlock;

    int x, y;

    oldBlock.width = newBlock.height;
    oldBlock.height = newBlock.width;

    for (x = 0; x < newBlock.width; x++) {
        for (y = 0; y < newBlock.height; y++) {
            oldBlock.data[x][y] = newBlock.data[newBlock.height - y - 1][x];
        }
    }

    x = givenState->activeBlockX;
    y = givenState->activeBlockY;

    givenState->activeBlockX -= (oldBlock.width - newBlock.width) / 2;
    givenState->activeBlockY -= (oldBlock.height - newBlock.height) / 2;
    givenState->activeBlock = oldBlock;

    if (tetrisCheckCollision(givenState)) {
        givenState->activeBlock = newBlock;
        givenState->activeBlockX = x;
        givenState->activeBlockY = y;
    }
}

void tetrisFallBlocks(TetrisGameState* givenState) {
	++givenState->activeBlockY;

	if (tetrisCheckCollision(givenState)) {
		--givenState->activeBlockY;
		tetrisPrintBlock(givenState);
		tetrisCreateBlock(givenState);
	}
}

void tetrisClearLine(TetrisGameState* givenState, int l) {
    int x, y;

    for (x = 0; x < givenState->boardWidth; x++) {
        for (y = l; y > 4; y--) {
            givenState->board[x][y] = givenState->board[x][y - 1];
        }
    }

    for (x = 0; x < givenState->boardWidth; x++) {
        givenState->board[x][0] = givenState->boardColor;
    }
}

void tetrisCheckLineComplete(TetrisGameState* givenState) {
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
            tetrisClearLine(givenState, y);
            int holder = (givenState->completedLines);
            if (givenState->difficulty == 0) holder = holder*50;
            else if (givenState->difficulty == 1) holder = holder*100;
            else holder = holder*200;
            printf("%d line(s) completed at %d\n", givenState->completedLines, holder);
            char showScore[80];
        	sprintf(showScore, "%d", holder);
            VGAwriteText(givenState->gameX-10, givenState->gameY+12, 14, givenState->boardColor, showScore);
            printf("a");
            y++;
        }
    }
}

void tetrisInputLeft(TetrisGameState* givenState) {
    --givenState->activeBlockX;
    if (tetrisCheckCollision(givenState)) ++givenState->activeBlockX;
}

void tetrisInputRight(TetrisGameState* givenState) {
    ++givenState->activeBlockX;
    if (tetrisCheckCollision(givenState)) --givenState->activeBlockX;
}

void tetrisPrintBoard(TetrisGameState* givenState) {
    //printf("drawing start (%d, %d)", givenState->gameX, givenState->gameY);
	if (givenState->pause) {
		VGAwriteText(givenState->gameX-(strlen("PAUSE GAME")),(givenState->gameY+(givenState->boardHeight)),0,2,"PAUSE GAME");
	} else {
		VGAwriteText(givenState->gameX-(strlen("PAUSE GAME")),(givenState->gameY+(givenState->boardHeight)),0,0,"PAUSE GAME");

    for (int x = 0; x < (givenState->boardWidth)*2; x++) {
      for (int y = 8; y < (givenState->boardHeight)*2; y++) {
				if (x/2 >= givenState->activeBlockX &&
					y/2 >= givenState->activeBlockY &&
					x/2 < (givenState->activeBlockX + givenState->activeBlock.width) &&
					y/2 < (givenState->activeBlockY + givenState->activeBlock.height) &&
					givenState->activeBlock.data[y/2 - givenState->activeBlockY]
										   [x/2 - givenState->activeBlockX] != ' ') {
					VGADrawColorBox(givenState->gameX+x,givenState->gameY-8+y, givenState->activeBlock.color);
				} else {
					VGADrawColorBox(givenState->gameX+x,givenState->gameY-8+y,givenState->board[x/2][y/2]);}
			}
		}
	}
}

void tetrisPause(TetrisGameState* givenState) {
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
TetrisGameState tetrisPopulate(int x, int y, int boardcolor, int width, int height, int difficult) {
	TetrisGameState state; //new game
	state.boardColor = boardcolor; //set board color
	state.nextBlock = rand() % tetrisBlockCount; //intize first block
	int color = (rand() % 15)+1;
	if (color == boardcolor || color == nextColor) color = (rand() % 15)+1;
	state.nextColor = color; //initize first color
	tetrisInitialize(&state, width, height, x, y,difficult);
	tetrisCreateBlock(&state);
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
	for (int i = 0; i < 13; i++) {
		for (int j = 0; j < 8; j++) {
    		VGADrawColorBox(33+i,24+j,1);
		}
	}
    VGAwriteText(34, 25, 14, 1, " 1 PLAYER");
    VGAwriteText(34, 27, 14, 1, " 2 PLAYERS");
    VGAwriteText(34, 29, 14, 1, "PRESS ENTER");
    VGAwriteText(34, 31, 14, 1, "  TO PLAY ");
	VGAwriteText(34, 25, 14, 1, ">");

	VGAwriteText(10,46, 14, 1, "Instructions: W to rotate; AD L/R; ESC to pause; ENTER to restart");
}

int moveMenu(int given) {
	int toReturn;
	if (given == 0) {
		if (arrowPos == 27) {
			VGAwriteText(34, 25, 14, 1, ">");
			VGAwriteText(34, 27, 1, 1, " ");
			toReturn = 25;
		} else {
			VGAwriteText(34, 27, 14, 1, ">");
			VGAwriteText(34, 25, 1, 1, " ");
			toReturn = 27;
		}
	} else if (given == 1) {
		printf("di1:%d",difficultyPos1);
		if (difficultyPos1 == 0 || (difficultyPos1 >= 16 && difficultyPos1 < 20)) {
			if (difficultyPos1 == 0) difficultyPos1 = 16;
			VGAwriteText(34, difficultyPos1, 1, 1, " ");
			difficultyPos1 = difficultyPos1+2;
			VGAwriteText(34, difficultyPos1, 14, 1, ">");
		} else if (difficultyPos1 == 20) {
			VGAwriteText(34, difficultyPos1, 1, 1, " ");
			difficultyPos1 = 16;
			VGAwriteText(34, difficultyPos1, 14, 1, ">");
		}
		toReturn = difficultyPos1;
	} else if (given == 2) {
		printf("di2:%d",difficultyPos2);
		if (difficultyPos2 == 0 || (difficultyPos2 >= 36 && difficultyPos2 < 40)) {
			if (difficultyPos2 == 0) difficultyPos2 = 36;
			VGAwriteText(34, difficultyPos2, 1, 1, " ");
			difficultyPos2 = difficultyPos2+2;
			VGAwriteText(34, difficultyPos2, 14, 1, ">");
		} else if (difficultyPos2 == 40) {
			VGAwriteText(34, difficultyPos2, 1, 1, " ");
			difficultyPos2 = 36;
			VGAwriteText(34, difficultyPos2, 14, 1, ">");
		}
		toReturn = difficultyPos2;
	}
	return toReturn;
}

void clearMenu() {
	for (int i = 0; i < 14; i++) {
		for (int j = 0; j < 28; j++) {
    		VGADrawColorBox(32+i,14+j,0);
		}
	}
}

void difficultyMenu(int player) {
	printf("arrowAt: %d, dP1:%d,dP2:%d;dif1:%d,dif2:%d\n",arrowPos, difficultyPos1, difficultyPos2,difficulty1,difficulty2);
	difficultyPos1 = 16;
	for (int i = 0; i < 13; i++) {
		for (int j = 0; j < 8; j++) {
			VGADrawColorBox(33+i,14+j,1);
		}
	}
	VGAwriteText(34, 14, 14, 1, " PLAYER 1 ");
	VGAwriteText(34, 16, 14, 1, "    EASY  ");
	VGAwriteText(34, 18, 14, 1, "    HARD  ");
	VGAwriteText(34, 20, 14, 1, " DIFFICULT");
	VGAwriteText(34, 16, 14, 1, ">");
	if (player == 2) {
		difficultyPos2 = 36;
		for (int i = 0; i < 13; i++) {
			for (int j = 0; j < 8; j++) {
				VGADrawColorBox(33+i,34+j,1);
			}
		}
		VGAwriteText(34, 34, 14, 1, " PLAYER 2 ");
		VGAwriteText(34, 36, 14, 1, "    EASY  ");
		VGAwriteText(34, 38, 14, 1, "    HARD  ");
		VGAwriteText(34, 40, 14, 1, " DIFFICULT");
		VGAwriteText(34, 36, 14, 1, ">");
	}
}

void keyInput(int x, int y, int boardcolor, int width, int height) {
	srand(time(NULL));
	gameMenu();


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
				if (players != 0) {
					if (timer != 0) {
						if(!state1.dead) {
					   if(!state1.pause) {
							tetrisFallBlocks(&state1);
							//printf("%ul \n",time(NULL));
							reset = 1;
							reset = 0;
					  }
						tetrisPrintBoard(&state1);
						tetrisCheckLineComplete(&state1);
					}
						if(!state2.dead) {
						if(!state2.pause) {
							tetrisFallBlocks(&state2);
							//printf("%ul \n",time(NULL));
							reset = 1;
							reset = 0;
						  }
							tetrisPrintBoard(&state2);
							tetrisCheckLineComplete(&state2);
						}

					}
				}

				for (int i = 0; i < 6; i++) {
					if (kbdbuf.keycode[i] != 0) {
						//printf("%x - ", kbdbuf.keycode[i]);
						if (kbdbuf.keycode[i] == 26) { //W
							if (players == 0) arrowPos = moveMenu(0);
							else if (difficulty1 > 0) tetrisRotateBlock(&state1);
							else difficultyPos1 = moveMenu(1);
						} else if (kbdbuf.keycode[i] == 82){//up
							if (players == 2 ) {
								tetrisRotateBlock(&state2);
								if (difficulty1 > 0 && difficulty2 > 0 ) tetrisRotateBlock(&state2);
								else difficultyPos2 = moveMenu(2);
							}
						} else if (kbdbuf.keycode[i] == 4){ //A
							if (players != 0) tetrisInputLeft(&state1);
						} else if (kbdbuf.keycode[i] == 80){//left
							 if (players == 2 ) tetrisInputLeft(&state2);
						} else if (kbdbuf.keycode[i] == 22) { //S
							if (players != 0) tetrisFallBlocks(&state1);
						} else if ( kbdbuf.keycode[i] == 81){ //down
							 if (players == 2 ) tetrisFallBlocks(&state2);
						} else if (kbdbuf.keycode[i] == 7) { //D
							//printf("arrowAt: %d, dP1:%d,dP2:%d;dif1:%d,dif2:%d\n",arrowPos, difficultyPos1, difficultyPos2,difficulty1,difficulty2);
							if (players != 0) tetrisInputRight(&state1);
						} else if ( kbdbuf.keycode[i] == 79){ //right
							 if (players == 2 ) tetrisInputRight(&state2);
						} else if (kbdbuf.keycode[i] == 41) { //esc
							if (players != 0) tetrisPause(&state1);
						} else if ( kbdbuf.keycode[i] == 83){//numlock
							 if (players == 2 ) tetrisPause(&state2);
						} else if (kbdbuf.keycode[i] == 40 || kbdbuf.keycode[i] == 88) { //enter
							//printf("arrowAt: %d, dP1:%d,dP2:%d;dif1:%d,dif2:%d\n",arrowPos, difficultyPos1, difficultyPos2,difficulty1,difficulty2);
							if (kbdbuf.keycode[i] == 40 && players != 0) {
								if (state1.dead) {
									printf("rest1");
									tetrisCleanup(&state1);
									state1.dead = 0;
									VGAwriteText(x-(strlen("GAME OVER")),(y+8+(height)),0,0,"GAME OVER");
								}
							}
							if (kbdbuf.keycode[i] == 88 && players == 2 ) {
								if (state2.dead) {
									printf("rest2");
									tetrisCleanup(&state2);
									state2.dead = 0;
									VGAwriteText(x+44-(strlen("GAME OVER")),(y+8+(height)),0,0,"GAME OVER");
								}
							}
							printf("%d ",kbdbuf.keycode[i]);
							if (players == 0) {
								if (arrowPos == 25) players = 1;
								else if (arrowPos == 27) players = 2;
							} else {
								if (players == 1) {
									if (difficultyPos1 != 0 && difficulty1 >= 0 && run1 == 0) {
										state1 = tetrisPopulate(x, y, boardcolor,width, height, difficulty1);
										run1=1;
									} else if (difficulty1 == 0) {
										printf("a \n");
										difficultyMenu(1);
										difficulty1 = -1;
									} else {
										if (difficultyPos1 == 16) difficulty1 = 1;
										else if (difficultyPos1 == 18) difficulty1 = 2;
										else if (difficultyPos1 == 20) difficulty1 = 3;
									}
								} else if (players == 2) {
									if (difficultyPos1 != 0 && difficulty1 >= 0 && difficultyPos2 != 0 && difficulty2 >= 0&& run1 == 0&& run2 == 0) {
										state1 = tetrisPopulate(x, y, boardcolor,width, height, difficulty1);
										state2 = tetrisPopulate(x+44, y, boardcolor,width, height, difficulty2);
										run1 =1;
										run2 = 1;
										VGAwriteText(20, 49, 14, 1, "Instructions: up to rotate; ARROWS L/R");
										VGAwriteText(20, 51, 14, 1, "NUMLOCK to pause; NUMPAD ENTER to restart");
									} else if (difficulty1 ==0 || difficulty2 == 0) {
										printf("b \n");
										difficultyMenu(2);
										difficulty1 = -1;
										difficulty2 = -1;
									} else {
										if (difficultyPos2 == 36) difficulty2 = 1;
										else if (difficultyPos2 == 38) difficulty2 = 2;
										else if (difficultyPos2 == 40) difficulty2 = 3;
										if (difficultyPos1 == 16) difficulty1 = 1;
										else if (difficultyPos1 == 18) difficulty1 = 2;
										else if (difficultyPos1 == 20) difficulty1 = 3;
									}
								}
							}
							if ((players==1 && difficulty1 > 0) || (players==2 && difficulty2 >0 && difficulty2 >0)) clearMenu();
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
