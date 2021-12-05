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
void TetrisPrintBoard(TetrisGameState* givenState, int givenX, int givenY);
void paintScreen();

void TetrisInitialize(TetrisGameState* givenState, int width, int height);
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
void TetrisMain(int boardWidth, int boardHeight);
