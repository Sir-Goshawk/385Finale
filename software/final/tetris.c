/*
 * tetris.c
 *
 *  Created on: Dec 1, 2021
 *      Author: kelvin3
 */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "tetris.h"


TetrisBlock TetrisGetRandomTetrisBlock()
{
    const int tetrisBlockCount = sizeof(tetrisBlocks) / sizeof(TetrisBlock);
    return tetrisBlocks[rand() % tetrisBlockCount];
}

void TetrisInitialize(TetrisGameState* state, int width, int height)
{
    int x, y;

    state->completedLines = 0;
    state->dead = 0;
    state->boardWidth = width;
    state->boardHeight = height;
    state->board = (char**)malloc(sizeof(char*) * width);

    for (x = 0; x < width; ++x)
    {
        state->board[x] = (char*)malloc(sizeof(char) * height);

        for (y = 0; y < height; ++y)
        {
            state->board[x][y] = ' ';
        }
    }
}

void TetrisCleanup(TetrisGameState* state)
{
    int x;

    for (x = 0; x < state->boardWidth; ++x)
    {
        free(state->board[x]);
    }

    free(state->board);
}

void TetrisPrintBoard(TetrisGameState* state)
{
    int x, y;

    for (x = 0; x < 30; ++x)
    {
        printf("\n");
    }

    printf("%d line(s) completed\n\n", state->completedLines);

    for (y = 0; y < state->boardHeight; ++y)
    {
        printf("|");

        for (x = 0; x < state->boardWidth; ++x)
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

    for (x = 0; x < state->boardWidth + 2; ++x)
    {
        printf("-");
    }

    printf("\n");
}

int TetrisCheckCollision(TetrisGameState* state)
{
    int x, y, X, Y;

    TetrisBlock b = state->activeBlock;

    for (x = 0; x < b.width; ++x)
    {
        for (y = 0; y < b.height; ++y)
        {
            X = state->activeBlockX + x;
            Y = state->activeBlockY + y;

            if (X < 0 || X >= state->boardWidth)
            {
                return 1;
            }

            if (b.data[y][x] != ' ')
            {
                if ((Y >= state->boardHeight) ||
                    (X >= 0 && X < state->boardWidth && Y >= 0 &&
                        state->board[X][Y] != ' '))
                {
                    return 1;
                }
            }
        }
    }

    return 0;
}

void TetrisCreateBlock(TetrisGameState* state)
{
    state->activeBlock = TetrisGetRandomTetrisBlock();

    state->activeBlockX =
        (state->boardWidth / 2) - (state->activeBlock.width / 2);

    state->activeBlockY = 0;

    if (TetrisCheckCollision(state))
    {
        state->dead = 1;
    }
}

void TetrisPrintBlock(TetrisGameState* state)
{
    TetrisBlock b = state->activeBlock;

    int x, y;

    for (x = 0; x < b.width; ++x)
    {
        for (y = 0; y < b.height; ++y)
        {
            if (b.data[y][x] != ' ')
            {
                state->board[state->activeBlockX + x][state->activeBlockY + y] =
                    b.data[y][x];
            }
        }
    }
}

void TetrisRotateBlock(TetrisGameState* state)
{
    TetrisBlock b = state->activeBlock;
    TetrisBlock s = b;

    int x, y;

    b.width = s.height;
    b.height = s.width;

    for (x = 0; x < s.width; ++x)
    {
        for (y = 0; y < s.height; ++y)
        {
            b.data[x][y] = s.data[s.height - y - 1][x];
        }
    }

    x = state->activeBlockX;
    y = state->activeBlockY;

    state->activeBlockX -= (b.width - s.width) / 2;
    state->activeBlockY -= (b.height - s.height) / 2;
    state->activeBlock = b;

    if (TetrisCheckCollision(state))
    {
        state->activeBlock = s;
        state->activeBlockX = x;
        state->activeBlockY = y;
    }
}

void TetrisFallBlocks(TetrisGameState* state)
{
	++state->activeBlockY;

	if (TetrisCheckCollision(state))
	{
		--state->activeBlockY;
		TetrisPrintBlock(state);
		TetrisCreateBlock(state);
	}
}

void TetrisClearLine(TetrisGameState* state, int l)
{
    int x, y;

    for (y = l; y > 0; --y)
    {
        for (x = 0; x < state->boardWidth; ++x)
        {
            state->board[x][y] = state->board[x][y - 1];
        }
    }

    for (x = 0; x < state->boardWidth; ++x)
    {
        state->board[x][0] = ' ';
    }
}

void TetrisCheckLineComplete(TetrisGameState* state)
{
    int x, y, l;

    for (y = state->boardHeight - 1; y >= 0; --y)
    {
        l = 1;

        for (x = 0; x < state->boardWidth && l; ++x)
        {
            if (state->board[x][y] == ' ')
            //if (state->colorValue[x][y] == ' ')
            {
                l = 0;
            }
        }

        if (l)
        {
            ++(state->completedLines);
            TetrisClearLine(state, y);
            printf("%d line(s) completed at %d\n\n", state->completedLines, y);
            ++y;
        }
    }
}

void TetrisInputLeft(TetrisGameState* state)
{
    --state->activeBlockX;

    if (TetrisCheckCollision(state))
    {
        ++state->activeBlockX;
    }
}

void TetrisInputRight(TetrisGameState* state)
{
    ++state->activeBlockX;

    if (TetrisCheckCollision(state))
    {
        --state->activeBlockX;
    }
}

void TetrisMain(int boardWidth, int boardHeight)
{
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

