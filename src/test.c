/*******************************************************************************
 * Stefan Bylund 2017
 *
 * A layer 2 drawing demo program for ZX Spectrum Next.
 *
 * zcc +zxn -subtype=nex -vn -SO3 -startup=30 -clib=sdcc_iy
 *   --max-allocs-per-node200000 -L<zxnext_layer2>/lib/sdcc_iy -lzxnext_layer2
 *   -I<zxnext_layer2>/include zxnext_draw_demo.c -o zxnext_draw_demo -create-app
 ******************************************************************************/

#include <arch/zxn.h>
#include <input.h>
#include <z80.h>
#include <intrinsic.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "lib/zxn/zxnext_layer2.h"
#include "lib/zxn/zxnext_sprite.h"

#pragma output CRT_ORG_CODE = 0x8184
#pragma output REGISTER_SP = 0xFF58
#pragma output CLIB_MALLOC_HEAP_SIZE = 0
#pragma output CLIB_STDIO_HEAP_SIZE = 0
#pragma output CLIB_FOPEN_MAX = -1

#include "esp.h"
#include "uart.h"

// #pragma output CRT_ORG_CODE = 0x6164
// #pragma output REGISTER_SP = 0xC000
// #pragma output CLIB_MALLOC_HEAP_SIZE = 0
// #pragma output CLIB_STDIO_HEAP_SIZE = 0
// #pragma output CLIB_FOPEN_MAX = -1
// #pragma printf = "%c %s"

/*
 * Define IDE_FRIENDLY in your C IDE to disable Z88DK C extensions and avoid
 * parser errors/warnings in the IDE. Do NOT define IDE_FRIENDLY when compiling
 * the code with Z88DK.
 */
#ifdef IDE_FRIENDLY
#define __z88dk_fastcall
#define __preserves_regs(...)
#endif

#define CLOUD_SPRITE_Y 40

#define printAt(col, row, str) printf("\x16%c%c%s", (col), (row), (str))

/*******************************************************************************
 * Type Definitions
 ******************************************************************************/

typedef struct sprite_info {
    uint8_t x; // X coordinate in pixels
    uint8_t y; // Y coordinate in pixels
    int8_t dx; // Horizontal displacement in pixels
    int8_t dy; // Vertical displacement in pixels
} sprite_info_t;

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

static void init_hardware(void);

static void init_isr(void);

static void create_start_screen(void);

static void init_tests(void);

static void select_test(void);

static void test_clear_screen(layer2_screen_t *screen);

static void test_load_screen(layer2_screen_t *screen);

static void test_load_screen_with_palette(layer2_screen_t *screen);

static void test_draw_pixel(layer2_screen_t *screen);

static void test_draw_line(layer2_screen_t *screen);

static void test_draw_rect(layer2_screen_t *screen);

static void test_fill_rect(layer2_screen_t *screen);

static void test_draw_text(layer2_screen_t *screen);

static void test_blit(layer2_screen_t *screen);

static void test_blit_transparent(layer2_screen_t *screen);

/*******************************************************************************
 * Variables
 ******************************************************************************/

static uint8_t test_number = 0;

static layer2_screen_t shadow_screen = {SHADOW_SCREEN};

static layer2_screen_t off_screen = {OFF_SCREEN, 0, 1, 3};

//static uint8_t tall_sprite[192];

static sprite_info_t testSprite = {120, 88, 1, 1};

#if 0
//16x16
static const uint8_t sprite_pattern[] =
{
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
    0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
    0x04, 0xFF, 0xFB, 0xFB, 0xFB, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
    0x04, 0xFF, 0xFB, 0xF5, 0xF5, 0xFB, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
    0x04, 0xFF, 0xFB, 0xF5, 0xA8, 0xA8, 0xFB, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
    0x04, 0xFF, 0xFF, 0xFB, 0xA8, 0x44, 0xA8, 0xFB, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
    0x04, 0x04, 0x04, 0xFF, 0xFB, 0xA8, 0x44, 0xA8, 0xFB, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
    0xE3, 0xE3, 0xE3, 0x04, 0xFF, 0xFB, 0xA8, 0x44, 0x44, 0xFB, 0xFF, 0x04, 0xE3, 0x04, 0xE3, 0xE3,
    0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0xFF, 0xFB, 0x44, 0x44, 0x44, 0xFB, 0xFF, 0x04, 0x4D, 0x04, 0xE3,
    0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0xFF, 0xFB, 0x44, 0x44, 0x44, 0x44, 0xFA, 0x4D, 0x04, 0xE3,
    0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0xFF, 0xFB, 0x44, 0xFF, 0xF5, 0x44, 0x04, 0xE3, 0xE3,
    0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0xFF, 0x44, 0xF5, 0xA8, 0x04, 0xE3, 0xE3, 0xE3,
    0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0xFA, 0x44, 0x04, 0xA8, 0x04, 0xE3, 0xE3,
    0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0x4D, 0x4D, 0x04, 0xE3, 0x04, 0xF5, 0x04, 0xE3,
    0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0x04, 0xE3, 0xE3, 0xE3, 0x04, 0xFA, 0x04,
    0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0x04
};
#endif

//16x16
static const uint8_t packet[] = 
{
    0xe3, 0xe3, 0xe3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xe3,
    0xff, 0x0f, 0x0f, 0x0f, 0xff, 0x0f, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xe3,
    0xff, 0x0f, 0x0f, 0x0f, 0xff, 0x0f, 0x0f, 0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xe3,
    0xff, 0x0f, 0x0f, 0x0f, 0xff, 0x0f, 0x0f, 0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xe3,
    0xff, 0x0f, 0x0f, 0x0f, 0xff, 0x0f, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xe3,
    0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
};

// static const uint8_t sprite[] =
// {
//     0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
//     0x04, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
//     0x04, 0xFF, 0xFB, 0xFB, 0xFB, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
//     0x04, 0xFF, 0xFB, 0xF5, 0xF5, 0xFB, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
//     0x04, 0xFF, 0xFB, 0xF5, 0xA8, 0xA8, 0xFB, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
//     0x04, 0xFF, 0xFF, 0xFB, 0xA8, 0x44, 0xA8, 0xFB, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
//     0x04, 0x04, 0x04, 0xFF, 0xFB, 0xA8, 0x44, 0xA8, 0xFB, 0xFF, 0x04, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3,
//     0xE3, 0xE3, 0xE3, 0x04, 0xFF, 0xFB, 0xA8, 0x44, 0x44, 0xFB, 0xFF, 0x04, 0xE3, 0x04, 0xE3, 0xE3,
//     0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0xFF, 0xFB, 0x44, 0x44, 0x44, 0xFB, 0xFF, 0x04, 0x4D, 0x04, 0xE3,
//     0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0xFF, 0xFB, 0x44, 0x44, 0x44, 0x44, 0xFA, 0x4D, 0x04, 0xE3,
//     0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0xFF, 0xFB, 0x44, 0xFF, 0xF5, 0x44, 0x04, 0xE3, 0xE3,
//     0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0xFF, 0x44, 0xF5, 0xA8, 0x04, 0xE3, 0xE3, 0xE3,
//     0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0xFA, 0x44, 0x04, 0xA8, 0x04, 0xE3, 0xE3,
//     0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0x4D, 0x4D, 0x04, 0xE3, 0x04, 0xF5, 0x04, 0xE3,
//     0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0x04, 0xE3, 0xE3, 0xE3, 0x04, 0xFA, 0x04,
//     0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0xE3, 0x04, 0x04
// };

// 12x12
static const uint8_t sprite[] = 
{
    0xe3, 0xe3, 0xe3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xff, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0x0f, 0xff, 0xe3, 0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0xff,
    0xff, 0x0f, 0x0f, 0x0f, 0xff, 0x0f, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0x0f, 0x0f, 0x0f,
    0xff, 0x0f, 0x0f, 0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0xff, 0x0f, 0x0f, 0xff,
    0x0f, 0x0f, 0x0f, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0xff, 0x0f, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0xff,
    0xff, 0x0f, 0x0f, 0x0f, 0xff, 0xff, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xff, 0x0f, 0x0f,
    0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xff, 0x0f, 0x0f, 0x0f, 0x0f, 0x0f,
    0x0f, 0xff, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe3, 0xe3, 0xe3,
};

static const uint8_t cloud[] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xb6, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xb6, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xb6, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xb6, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xe3, 0xb6, 0xb6, 0xff, 0xff, 0xff, 0xff, 0xb6, 0xb6, 0xe3, 0xb6, 0xff, 0xff, 0xff, 0xff, 0xb6,
    0xe3, 0xe3, 0xe3, 0xb6, 0xb6, 0xb6, 0xb6, 0xe3, 0xe3, 0xe3, 0xe3, 0xb6, 0xb6, 0xb6, 0xb6, 0xe3,
};

#if 0
static const uint8_t specNext[] =  // 8 x 16
{
	 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	 0x0, 0x80, 0x80, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
	 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0xc0, 0xc0,
	 0x0, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0xfc, 0xfc,
	 0x0, 0xff, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0x18, 0x18,
	 0x0, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0, 0x0, 0x6, 0x6,
	 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0,
};

static const uint8_t server[] = 
{  // 26x21
    0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d,
    0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x6d,
    0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d,
    0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e,
    0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0x6d, 0x6d, 0x8e, 0xff, 0xff, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e,
    0x8e, 0x8e, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x8e, 0xff, 0xff,
    0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x6d, 0x6d, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e,
    0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0x6d,
    0x6d, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e,
    0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e,
    0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e,
    0x8e, 0x8e, 0x8e, 0x8e, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x8e,
    0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x6d, 0x6d,
    0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e,
    0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0x6d, 0x6d, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e, 0x8e,
    0x8e, 0x8e, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d,
    0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d,
    0x6d, 0x6d, 0x6d, 0x6d, 0xe1, 0x6d, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0x6d, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3, 0xe3,
    0xe3, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d,
    0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0x6d, 0xe1, 0x6d, 0xf8, 0x6d,
    0x18, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25, 0x25,
    0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d,
    0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0x6d,
    0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x25, 0x25, 0x25,
    0x25, 0x25, 0x25, 0x25, 0x6d, 0x6d, 0xe3, 0xe3, 0xe3, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d,
    0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d,
    0xe3, 0xe3,
};
#endif

struct coord
{
    uint8_t x;
    uint8_t y; 
};

static struct coord pos[64];

/*******************************************************************************
 * Functions
 ******************************************************************************/

// ESP DETECT BPS

void main_esp_detect_bps(void)
{
   printf("Detecting ESP baud rate\n");
   
   if (!esp_detect_bps())
   {
      esp_bps = 115200UL;
      printf("\n  Failed, selecting default");
   }

   printf("\n  Setting uart to %lu\n", esp_bps);
   uart_set_prescaler(uart_compute_prescaler(esp_bps));
}

static void init_hardware(void)
{
    // Put Z80 in 28 MHz turbo mode.
    ZXN_NEXTREG(REG_TURBO_MODE, 0x03);

    // Disable RAM memory contention.
    ZXN_NEXTREGA(REG_PERIPHERAL_3, ZXN_READ_REG(REG_PERIPHERAL_3) | RP3_DISABLE_CONTENTION);

    layer2_set_main_screen_ram_bank(8);
    layer2_set_shadow_screen_ram_bank(11);
}

static void init_isr(void)
{
    // Set up IM2 interrupt service routine:
    // Put Z80 in IM2 mode with a 257-byte interrupt vector table located
    // at 0x8000 (before CRT_ORG_CODE) filled with 0x81 bytes. Install an
    // empty interrupt service routine at the interrupt service routine
    // entry at address 0x8181.

    intrinsic_di();
    im2_init((void *) 0x8000);
    memset((void *) 0x8000, 0x81, 257);
    z80_bpoke(0x8181, 0xFB);
    z80_bpoke(0x8182, 0xED);
    z80_bpoke(0x8183, 0x4D);
    intrinsic_ei();
}

static void create_start_screen(void)
{
    int c, k;

    zx_border(INK_YELLOW);
    zx_cls(BRIGHT | INK_BLACK | PAPER_WHITE);

    ZXN_NEXTREG(0x07, 0x03);  // 28MHz
   
    IO_153B = 0x00;  // Select ESP for UART(?)

    // Reset ESP
    // ZXN_NEXTREG(0x02, 0x80);
    // z80_delay_ms(100*8);       // 100ms, about 8x longer for 28MHz
    // XN_NEXTREG(0x02, 0);
    // z80_delay_ms(8000U*8U);      // 8s, about 8x longer for 28MHz

    printf("Uart init\r\n");
    esp_response_time_ms = 66 + ESP_FW_RESPONSE_TIME_MS;   // two bit periods at 300bps
    uart_rx_readline_last(buffer, sizeof(buffer)-1);   // clear Rx

    esp_response_time_ms = 66 + ESP_FW_RESPONSE_TIME_MS;   // two bit periods at 300bps
   
    printf("ESP AT+GMR follows...\n");
    uart_rx_readline_last(buffer, sizeof(buffer)-1);   // clear Rx

    // Send AT command to UART.
    uart_tx(STRING_ESP_TX_AT_GMR);
    *buffer = 0;

    // Read response from UART. Print to to screen.
    do
    {
        puts(buffer);  
        *buffer = 0;
        uart_rx_readline(buffer, sizeof(buffer)-1);
    }
    while (*buffer);

    printf("Press any key to start\n");
    //printAt(1, 15, "Press any key to switch screen");
}

static void create_sprites(void)
{
    set_sprite_slot(0);
    set_sprite_pattern(packet);

    set_sprite_slot(0);
    set_sprite_attributes_ext(0, testSprite.x, testSprite.y, 0, 0, true);
}

static void update_sprites(void)
{
    // Calculate next position of sprite.
    testSprite.x += testSprite.dx;
    testSprite.y += testSprite.dy;

    // If sprite is at the edge of the screen then change its direction.
    if ((testSprite.x == 0) || (testSprite.x >= 240))
    {
        testSprite.dx = -testSprite.dx;
    }
    if ((testSprite.y == 0) || (testSprite.y >= 176))
    {
        testSprite.dy = -testSprite.dy;
    }

    // Update sprite position.
    set_sprite_slot(0);
    set_sprite_attributes_ext(0, testSprite.x, testSprite.y, 0, 0, true);
}


static void DrawCloudEdge(layer2_screen_t *screen)
{
    // Draw tiled cloud
    int i=0;
    int x=0;
    for(i=0; i<16; i++ )
    {
        layer2_blit_transparent(x, CLOUD_SPRITE_Y,  cloud, 16, 26, screen); // top
        x+=16;
    }
}

static void DrawPacket(layer2_screen_t *screen)
{
    layer2_blit_transparent(120+pos[7].x, 24+pos[7].y,  sprite, 12, 12, screen); // top

}

static void DrawGame(void)
{
    for(int i=0; i<64; i++)
    {
        ++pos[i].x; 
        ++pos[i].y; 
    }

    // Draw top part with white (cloud)
    layer2_fill_rect(0, 0,  256, CLOUD_SPRITE_Y, 0xff, &off_screen);

    // Fill the lower screen area with black.
    layer2_fill_rect(0, CLOUD_SPRITE_Y, 256, (int)(194-CLOUD_SPRITE_Y), 0, &off_screen);

    DrawPacket(&off_screen);

    DrawCloudEdge(&off_screen);

    //test_draw_text(&off_screen);

    // Swap the double buffered screen.
    if (IS_SHADOW_SCREEN(&off_screen))
    {
        layer2_flip_main_shadow_screen();
    }
    else if (IS_OFF_SCREEN(&off_screen))
    {
        layer2_copy_off_screen(&off_screen);
    }
}


int main(void)
{
    init_hardware();
    init_isr();
    create_sprites();
    set_sprite_layers_system(true, false, LAYER_PRIORITIES_S_L_U, false);

    create_start_screen();
    //in_wait_key();

    //layer2_configure(true, false, false, 0);
    //layer2_draw_text(3,  12, "Hello", 0xEF, &off_screen);

    while (true)
    {
        //DrawGame();

        // Wait for vertical blanking interval.
        intrinsic_halt();
        update_sprites();

 
        // if (in_inkey())
        // {
        //     in_wait_nokey();
        //     select_test();
        // }
    }

    // Trig a soft reset. The Next hardware registers and I/O ports will be reset by NextZXOS after a soft reset.
    ZXN_NEXTREG(REG_RESET, RR_SOFT_RESET);
    return 0;
}