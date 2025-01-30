// The text tilemap for drawing status and info texts efficiently. This draws to a tilemap in
// the ULA layer and the normal ULA layer is disabled. 

// Hannu Viitala, 2025
// Based on QEtoo created by D Rimron-Soutter on 07/10/2020.
// Above based on libcuss by David Given dg@cowlark.com

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEXTMODE_ULA_BEHIND             0x00
#define TEXTMODE_ULA_FRONT              0x01

#define TEXTMODE_DEFAULT_COLOUR         0x04
#define TEXTMODE_REVERSE_COLOUR         0x84

#define TT_COLOR_TRANS 0
#define TT_COLOR_RED 2
#define TT_COLOR_GREEN 4 
#define TT_COLOR_YELLOW 6
#define TT_COLOR_BLUE 8
#define TT_COLOR_MAGENTA 10
#define TT_COLOR_CYAN 12
#define TT_COLOR_LIGHT_GREY 14 

#define TT_COLOR_GREY 16  // ?
#define TT_COLOR_PINK 18
#define TT_COLOR_LIGHT_GREEN 20
#define TT_COLOR_LIGHT_YELLOW 22
#define TT_COLOR_LIGHT_BLUE 24  // the same as TT_COLOR_BLUE
#define TT_COLOR_LIGHT_MAGENTA 26  // "red"   
#define TT_COLOR_LIGHT_CYAN 28  
#define TT_COLOR_WHITE 30  

struct __tilemap
{
    unsigned char tile;  // The tile index i.e. the "char code". 
    unsigned char flags;  // The flags for the char.
};

extern struct __tilemap* tilemap;

struct __tiles
{
    unsigned char bmp[8];  // Each tile image is 8 bytes (8x8 px, 1bpp)
};

extern struct __tiles* tiles; 
extern uint16_t screenx;
extern uint16_t screeny;
extern uint16_t screencolour;

void TextTileMapClear(void);
void TextTileMapClearToEol(void);
void TextTileMapPutsPos(uint16_t row, uint16_t col, char* s);
void TextTileMapPuts(char* s);
void TextTileMapPutcPos(uint16_t row, uint16_t col, uint16_t c);
void TextTileMapPutc(uint16_t c);
void TextTileMapGoto(uint16_t row, uint16_t col);
void TextTileMapPutColorOnlyPos(uint16_t row, uint16_t col);

#define TEXTTILEMAP_SCREENWIDTH 80  // The width of the text tilemap layer in tiles.
#define TEXTTILEMAP_SCREENHEIGHT 32 // The height of the text tilemap layer in tiles.


