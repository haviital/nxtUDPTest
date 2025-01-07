 //
// Created by D Rimron-Soutter on 07/10/2020.
//

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEXTMODE_ULA_BEHIND             0x00
#define TEXTMODE_ULA_FRONT              0x01

#define TEXTMODE_DEFAULT_COLOUR         0x04
#define TEXTMODE_REVERSE_COLOUR         0x84

struct __tilemap
{
    unsigned char tile;
    unsigned char flags;
};

extern struct __tilemap* tilemap;

struct __tiles
{
    unsigned char bmp[8];           // each tile image is 8 bytes (64px@1bpp)
};

extern struct __tiles* tiles;   //
extern uint16_t screenx;
extern uint16_t screeny;
extern uint16_t screencolour;

void TextTileMapClear(void);
void TextTileMapClearToEol(void);
void TextTileMapPutsPos(uint16_t x, uint16_t y, const char* s);
void TextTileMapPuts(const char* s);
void TextTileMapPutcPos(uint16_t x, uint16_t y, uint16_t c);
void TextTileMapPutc(uint16_t c);
void TextTileMapGoto(uint16_t x, uint16_t y);

#define TEXTTILEMAP_SCREENWIDTH 40
#define TEXTTILEMAP_SCREENHEIGHT 32


