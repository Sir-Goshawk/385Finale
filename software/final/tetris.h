/*
 * tetris.h
 *
 *  Created on: Dec 1, 2021
 *      Author: kelvin3
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>


typedef struct
{
    int width;
    int height;
    char data[5][5];
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
} TetrisGameState;


TetrisBlock TetrisGetRandomTetrisBlock();
void TetrisInitialize(TetrisGameState* state, int width, int height);
void TetrisCleanup(TetrisGameState* state);
void TetrisPrintBoard(TetrisGameState* state);
int TetrisCheckCollision(TetrisGameState* state);
void TetrisCreateBlock(TetrisGameState* state);
void TetrisPrintBlock(TetrisGameState* state);
void TetrisRotateBlock(TetrisGameState* state);
void TetrisFallBlocks(TetrisGameState* state);
void TetrisClearLine(TetrisGameState* state, int l);
void TetrisCheckLineComplete(TetrisGameState* state);
void TetrisInputLeft(TetrisGameState* state);
void TetrisInputRight(TetrisGameState* state);
void TetrisMain(int boardWidth, int boardHeight);

