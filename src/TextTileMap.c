// The text tilemap for drawing status and info texts efficiently. This draws to a tilemap in
// the ULA layer and the normal ULA layer is disabled. 

// Hannu Viitala, 2025
// Based on QEtoo created by D Rimron-Soutter on 07/10/2020.
// Above based on libcuss by David Given dg@cowlark.com

// Writing via layer2_draw_text() to the layer2 is so slow that it cannot be used 
// in the game loop.

#include <arch/zxn.h>
#include <input.h>
#include <z80.h>

#include <intrinsic.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "lib/zxn/zxnext_layer2.h"
#include "lib/zxn/zxnext_sprite.h"
#include "defines.h"
#include "TextTileMap.h"

struct __tilemap* tilemap = (struct __tilemap*)0x6C00;  // The tilemap data which has the attributes and char code.
struct __tiles* tiles = (struct __tiles*)0x5C00;  // The tile bitmap data (copied from ROM).
uint16_t screenx = 0, screeny = 0;
uint16_t screencolour = TEXTMODE_DEFAULT_COLOUR;

// Draw the char at.
void TextTileMapPutcPos(uint16_t row, uint16_t col, uint16_t c)
{
	screenx = col;
	screeny = row;
    TextTileMapPutc(c);
}

// Draw the char.
void TextTileMapPutc(uint16_t c)
{
    if (screeny >= TEXTTILEMAP_SCREENHEIGHT)
            return;

    if (c < 32)
    {
            TextTileMapPutc('^');
            c += '@' - 32;
    }
    else
    {
        c-=32;
    }

    tilemap[screeny*TEXTTILEMAP_SCREENWIDTH+screenx].tile = c;
    tilemap[screeny*TEXTTILEMAP_SCREENWIDTH+screenx].flags = screencolour;
    screenx++;
    if (screenx == TEXTTILEMAP_SCREENWIDTH)
    {
            screenx = 0;
            screeny++;
    }
}

// Draw the char.
void TextTileMapPutColorOnlyPos(uint16_t row, uint16_t col)
{
    if (row >= TEXTTILEMAP_SCREENHEIGHT)
            return;

	screenx = col;
	screeny = row;
    tilemap[screeny*TEXTTILEMAP_SCREENWIDTH+screenx].flags = screencolour;
}

// Draw the string at.
void TextTileMapPutsPos(uint16_t row, uint16_t col, char* s)
{
	screenx = col;
	screeny = row;
    TextTileMapPuts(s);
}

// Draw the string.
void TextTileMapPuts(char* s)
{
	for (;;)
	{
		uint16_t c = *s++;
		if (!c)
			break;
		TextTileMapPutc(c);
	}
}

// Draw the spaces until the end of line.
void TextTileMapClearToEol(void)
{
    if (screeny >= TEXTTILEMAP_SCREENHEIGHT)
            return;

    uint16_t i = TEXTTILEMAP_SCREENWIDTH - screenx;
    if ((i != 0) && (screeny == (TEXTTILEMAP_SCREENHEIGHT-1)))
        i--;

    while (i--) {
        tilemap[(screeny*TEXTTILEMAP_SCREENWIDTH) + TEXTTILEMAP_SCREENWIDTH-i-1].tile = (' ' -32);
    }

    TextTileMapGoto(screeny, screenx);
}

// Set all chars to zero for the whole screen.
void TextTileMapClear(void)
{
    for(int i=0; i < TEXTTILEMAP_SCREENHEIGHT*TEXTTILEMAP_SCREENWIDTH; i++)
    {
        tilemap[i].tile = 0;
        tilemap[i].flags = 0;
    }
    screenx = screeny = 0;
}

// Goto to the position.
void TextTileMapGoto(uint16_t row, uint16_t col)
{
	screenx = col;
	screeny = row;
}
