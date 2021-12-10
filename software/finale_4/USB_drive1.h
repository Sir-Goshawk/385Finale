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


typedef struct {
    int width;
    int height;
    char data[5][5];
    int color;
} TetrisBlock;

// clang-format off
const TetrisBlock tetrisBlocks[] = {
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


typedef struct {
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
typedef struct {
    int totalChance;
    int cumulativeChances[7];
} TetrisBlockChanceTable;




BYTE GetDriverandReport();
void setLED(int LED);
void clearLED(int LED);
void printSignedHex0(signed char value);
void printSignedHex1(signed char value);
void setKeycode(WORD keycode);

void keyInput(int x, int, int boardcolor, int width, int height);
void tetrisPrintBoard(TetrisGameState* givenState);

void tetrisInitialize(TetrisGameState* givenState, int width, int height, int givenX, int givenY, int difficult);
void tetrisCleanup(TetrisGameState* givenState);
void tetrisPrintBoard(TetrisGameState* givenState);
int tetrisCheckCollision(TetrisGameState* givenState);
void tetrisCreateBlock(TetrisGameState* givenState);
void tetrisPrintBlock(TetrisGameState* givenState);
void tetrisRotateBlock(TetrisGameState* givenState);
void tetrisFallBlocks(TetrisGameState* givenState);
void tetrisClearLine(TetrisGameState* givenState, int l);
void tetrisCheckLineComplete(TetrisGameState* givenState);
void tetrisInputLeft(TetrisGameState* givenState);
void tetrisInputRight(TetrisGameState* givenState);
void tetrisShowScore(int x, int y);
void tetrisPause(TetrisGameState* givenState);
TetrisGameState tetrisPopulate(int x, int y, int boardcolor, int width, int height, int difficult);
void gameMenu();
int moveMenu(int given);
void clearMenu();
void difficultyMenu(int player);
void tetrisInitializeChanceTable(TetrisBlockChanceTable* table, int* chances);
int tetrisGetRandomTetrisBlock(TetrisGameState* givenState);
