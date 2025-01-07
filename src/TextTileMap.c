 
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

struct __tilemap* tilemap = (struct __tilemap*)0x6C00;
struct __tiles* tiles = (struct __tiles*)0x5C00;
uint16_t screenx = 0, screeny = 0;
uint16_t screencolour = TEXTMODE_DEFAULT_COLOUR;

void TextTileMapPutcPos(uint16_t x, uint16_t y, uint16_t c)
{
	screenx = x;
	screeny = y;
    TextTileMapPutc(c);
}

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

void TextTileMapPutsPos(uint16_t x, uint16_t y, const char* s)
{
	screenx = x;
	screeny = y;
    TextTileMapPuts(s);
}

void TextTileMapPuts(const char* s)
{
	for (;;)
	{
		uint16_t c = *s++;
		if (!c)
			break;
		TextTileMapPutc(c);
	}
}

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

    TextTileMapGoto(screenx, screeny);
}

void TextTileMapClear(void)
{
    //memset(tilemap, 0, sizeof(tilemap));
    memset(tilemap, 0, sizeof(tilemap));
    screenx = screeny = 0;
}

void TextTileMapGoto(uint16_t x, uint16_t y)
{
	screenx = x;
	screeny = y;
}
