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

#pragma output CRT_ORG_CODE = 0x6164
#pragma output REGISTER_SP = 0xC000
#pragma output CLIB_MALLOC_HEAP_SIZE = 0
#pragma output CLIB_STDIO_HEAP_SIZE = 0
#pragma output CLIB_FOPEN_MAX = -1
#pragma printf = "%c %s"

/*
 * Define IDE_FRIENDLY in your C IDE to disable Z88DK C extensions and avoid
 * parser errors/warnings in the IDE. Do NOT define IDE_FRIENDLY when compiling
 * the code with Z88DK.
 */
#ifdef IDE_FRIENDLY
#define __z88dk_fastcall
#define __preserves_regs(...)
#endif

#define printAt(col, row, str) printf("\x16%c%c%s", (col), (row), (str))

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

static uint8_t tall_sprite[192];

static const uint8_t sprite[] =
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

struct coord
{
    uint8_t x;
    uint8_t y; 
};

static struct coord pos[64];

/*******************************************************************************
 * Functions
 ******************************************************************************/

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
    // at 0x6000 (before CRT_ORG_CODE) filled with 0x61 bytes. Install an
    // empty interrupt service routine at the interrupt service routine
    // entry at address 0x6161.

    intrinsic_di();
    im2_init((void *) 0x6000);
    memset((void *) 0x6000, 0x61, 257);
    z80_bpoke(0x6161, 0xFB);
    z80_bpoke(0x6162, 0xED);
    z80_bpoke(0x6163, 0x4D);
    intrinsic_ei();
}

static void create_start_screen(void)
{
    int c, k;

    zx_border(INK_WHITE);
    zx_cls(INK_BLACK | PAPER_WHITE);

    printf("Uart inited\r\n");

    printAt(5,  7, "Press any key to start");
    printAt(1, 15, "Press any key to switch screen");
}

static void init_tests(void)
{
    zx_border(INK_YELLOW);
    zx_cls(INK_BLACK | PAPER_WHITE);

    memset(tall_sprite, 0x27, 192);
    layer2_configure(true, false, false, 0);
}

static void select_test(void)
{
    for(int i=0; i<64; i++)
    {
        ++pos[i].x; 
        ++pos[i].y; 
    }

    test_blit_transparent(&off_screen);

    test_draw_text(&off_screen);
}

static void test_draw_text(layer2_screen_t *screen)
{
    // layer2_fill_rect(0, 0,   256, 64, 0xFE, screen);
    // layer2_fill_rect(0, 64,  256, 64, 0x7E, screen);
    // layer2_fill_rect(0, 128, 256, 64, 0x9F, screen);

    layer2_draw_text(3+pos[0].x,  12+pos[0].y, "Hello", 0xEF, screen);
    layer2_set_font(NULL);
    layer2_draw_text(11-pos[1].x, 12-pos[1].y, "Hello", 0xEF, screen);
    layer2_set_font((void *) 0x3D00);

    layer2_draw_text(19+pos[2].x, 12+pos[2].y, "Hello", 0xEF, screen);

    // Clipped text.
    layer2_draw_text(3-pos[3].x,  29-pos[3].y, "Hello", 0xEF, screen);
    layer2_draw_text(11+pos[4].x, 29+pos[4].y, "Hello", 0xEF, screen);
    layer2_draw_text(19-pos[5].x, 29-pos[5].y, "Hello", 0xEF, screen);

    // Outside screen.
    layer2_draw_text(24+pos[6].x, 32+pos[6].y, "Hello", 0xEF, screen);

    if (IS_SHADOW_SCREEN(screen))
    {
        layer2_flip_main_shadow_screen();
    }
    else if (IS_OFF_SCREEN(screen))
    {
        layer2_copy_off_screen(screen);
    }
}

static void test_blit_transparent(layer2_screen_t *screen)
{
    layer2_fill_rect(0, 0,   256, 64, 0x00, screen);
    layer2_fill_rect(0, 64,  256, 64, 0x00, screen);
    layer2_fill_rect(0, 128, 256, 64, 0x00, screen);

    layer2_blit_transparent(120+pos[7].x, 24+pos[7].y,  sprite, 16, 16, screen); // top
    layer2_blit_transparent(120, 56,  sprite, 16, 16, screen); // top + middle
    layer2_blit_transparent(120, 88,  sprite, 16, 16, screen); // middle
    layer2_blit_transparent(120, 120, sprite, 16, 16, screen); // middle + bottom
    layer2_blit_transparent(120, 152, sprite, 16, 16, screen); // bottom
    layer2_blit_transparent(120, 184, sprite, 16, 16, screen); // bottom clipped

    layer2_blit_transparent(64, 48, tall_sprite, 2, 96, screen); // top + middle + bottom

    // Clipped blits.
    layer2_blit_transparent(248, 56,  sprite, 16, 16, screen);
    layer2_blit_transparent(248, 120, sprite, 16, 16, screen);
    layer2_blit_transparent(248, 184, sprite, 16, 16, screen);

    // Outside screen.
    layer2_blit_transparent(248, 248, sprite, 16, 16, screen);

    if (IS_SHADOW_SCREEN(screen))
    {
        layer2_flip_main_shadow_screen();
    }
    else if (IS_OFF_SCREEN(screen))
    {
        layer2_copy_off_screen(screen);
    }
}

int main(void)
{
    init_hardware();
    init_isr();

    create_start_screen();
    in_wait_key();
    init_tests();

    while (true)
    {
        select_test();
 
        // if (in_inkey())
        // {
        //     in_wait_nokey();
        //     select_test();
        // }
    }
}