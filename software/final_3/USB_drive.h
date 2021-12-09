/*
 * USB_drive.h
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
        4, // width
        4, // height
        {"XXXX",
		 "XXXX",
		 "XXXX",
		 "XXXX"},
    },
    {
        6, // width
        4, // height
        {"  XX  ",
		 "  XX  ",
         "XXXXXX",
         "XXXXXX"},
    },
    {
        8, // width
        2, // height
        {"XXXXXXXX",
         "XXXXXXXX"},
    },
    {
        4, // width
        6, // height
        {"XXXX",
         "XXXX",
         "XX  ",
         "XX  ",
         "XX  ",
         "XX  ",},
    },
    {
        4, // width
        6, // height
        {"XXXX",
         "XXXX",
         "  XX",
         "  XX",
         "  XX",
         "  XX",},
    },
    {
        6, // width
        4, // height
        {"XXXX  ",
         "XXXX  ",
         "  XXXXX",
         "  XXXXX"},
    },
	{
		6,
		4,
		{"  XXXX",
		 "  XXXX",
		 "XXXX  ",
		 "XXXX  "}
	}
};
// clang-format on


typedef struct
{
    char** board;
    int boardWidth;
    int boardHeight;
    int dead;
    int completedLines;
    TetrisBlock activeBlock;
    int activeBlockX;
    int activeBlockY;
    int pause;
} TetrisGameState;




BYTE GetDriverandReport();
void setLED(int LED);
void clearLED(int LED);
void printSignedHex0(signed char value);
void printSignedHex1(signed char value);
void setKeycode(WORD keycode);

void keyInput(int x, int, int boardcolor, int width, int height);
void TetrisPrintBoard(TetrisGameState* givenState);

void TetrisInitialize(TetrisGameState* givenState, int width, int height, int givenX, int givenY);
void TetrisCleanup(TetrisGameState* givenState);
void TetrisPrintBoard(TetrisGameState* givenState);
int TetrisCheckCollision(TetrisGameState* givenState);
void TetrisCreateBlock(TetrisGameState* givenState);
void TetrisPrintBlock(TetrisGameState* givenState);
void TetrisRotateBlock(TetrisGameState* givenState);
void TetrisFallBlocks(TetrisGameState* givenState);
void TetrisClearLine(TetrisGameState* givenState, int l);
void TetrisCheckLineComplete(TetrisGameState* givenState);
void TetrisInputLeft(TetrisGameState* givenState);
void TetrisInputRight(TetrisGameState* givenState);
void TetrisShowScore(int x, int y);
void TetrisPause(TetrisGameState* givenState);
TetrisGameState TetrisPopulate(int x, int y, int boardcolor, int width, int height);
void gameMenu();
void moveMenu();
void clearMenu()
