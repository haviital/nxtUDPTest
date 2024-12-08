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


#pragma output CRT_ORG_CODE = 0x6164
#pragma output REGISTER_SP = 0xC000
#pragma output CLIB_MALLOC_HEAP_SIZE = 0
#pragma output CLIB_MALLOC_HEAP_SIZE = 0
#pragma output CLIB_STDIO_HEAP_SIZE = 0
#pragma output CLIB_FOPEN_MAX = -1

#include "esp.h"
#include "uart.h"
#include "GameObject.h"
#include "gfx.h"

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

/*******************************************************************************
 * Macros
 ******************************************************************************/
#define printInk(k)                 printf("\x10%c", '0'+k)
#define printPaper(k)               printf("\x11%c", '0'+k)
#define printBright(k)              printf("\x13%d", k);
#define printAt(row, col)           printf("\x16%c%c", (col)+1, (row)+1)
#define printStrAt(row, col, str)   printf("\x16%c%c%s", (col)+1, (row)+1, str)

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

#define INCOMING_PACKET_GOB_COUNT 10
static GameObject incomingPacketGobs[INCOMING_PACKET_GOB_COUNT];

#define OUTGOING_PACKET_GOB_COUNT 10
static GameObject outgoingPacketGobs[OUTGOING_PACKET_GOB_COUNT];

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
    im2_init((void *) 0x6000);
    memset((void *) 0x6000, 0x61, 257);
    z80_bpoke(0x6161, 0xFB);
    z80_bpoke(0x6162, 0xED);
    z80_bpoke(0x6163, 0x4D);
    intrinsic_ei();
}

static void create_start_screen(void)
{
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

#define CLOUD_PATTERN_SLOT 0
static void create_sprites(void)
{
    // *** Static sprites

    // Cloud sprite 0
    set_sprite_slot(100);
    set_sprite_pattern(cloudSpr);
    set_sprite_slot(100);
    set_sprite_attributes_ext(100, 30, 40, 0, 0, true);

    // Cloud sprite 1
    set_sprite_slot(101);
    set_sprite_pattern(cloudSpr);
    set_sprite_slot(101);
    set_sprite_attributes_ext(101, 216, 40, 0, 0, true);

    // Server sprite 0
    set_sprite_slot(102);
    set_sprite_pattern(serverSpr0);
    set_sprite_slot(102);
    set_sprite_attributes_ext(102, 208, 154, 0, 0, true);

    // Server sprite 1
    set_sprite_slot(103);
    set_sprite_pattern(serverSpr1);
    set_sprite_slot(103);
    set_sprite_attributes_ext(103, (int)(208+16), 154, 0, 0, true);

    // Server sprite 2
    set_sprite_slot(104);
    set_sprite_pattern(serverSpr2);
    set_sprite_slot(104);
    set_sprite_attributes_ext(104, 208, (int)(154+16), 0, 0, true);

    // Server sprite 3
    set_sprite_slot(105);
    set_sprite_pattern(serverSpr3);
    set_sprite_slot(105);
    set_sprite_attributes_ext(105, (int)(208+16), (int)(154+16), 0, 0, true);

    // SpecNext sprite 0
    set_sprite_slot(106);
    set_sprite_pattern(specnextSpr0);
    set_sprite_slot(106);
    set_sprite_attributes_ext(106, 28, (int)(154+10), 0, 0, true);

    // SpecNext sprite 1
    set_sprite_slot(107);
    set_sprite_pattern(specnextSpr1);
    set_sprite_slot(107);
    set_sprite_attributes_ext(107, 28+16, (int)(154+10), 0, 0, true);

    // *** Moving sprites

    // Incoming data packet gobs
    GameObject defaultGob = {.x = 30, .y= 88, .sx=0, .sy=3, .spriteIndex=0, .spritePatternIndex = CLOUD_PATTERN_SLOT, .isHidden=true};
    for(int i=0; i<INCOMING_PACKET_GOB_COUNT; i++)
    {
        int sprIndex = i;
        incomingPacketGobs[i] = defaultGob;
        incomingPacketGobs[i].y = i*32;
        incomingPacketGobs[i].spriteIndex = sprIndex;
        set_sprite_slot(sprIndex);
        set_sprite_pattern(packet);
        set_sprite_slot(sprIndex);
        set_sprite_attributes_ext(incomingPacketGobs[i].spritePatternIndex, incomingPacketGobs[i].x, incomingPacketGobs[i].y, 0, 0, true);
    }

    // Outgoing data packet gobs
    for(int i=0; i<OUTGOING_PACKET_GOB_COUNT; i++)
    {
        int sprIndex = i + INCOMING_PACKET_GOB_COUNT;
        outgoingPacketGobs[i] = defaultGob;
        outgoingPacketGobs[i].x = 216;
        outgoingPacketGobs[i].y = i*32;
        outgoingPacketGobs[i].sy = -outgoingPacketGobs[i].sy;
        outgoingPacketGobs[i].spriteIndex = sprIndex;
        //set_sprite_slot(i);
        //set_sprite_pattern(packet);
        set_sprite_slot(sprIndex);
        set_sprite_attributes_ext(outgoingPacketGobs[i].spritePatternIndex, outgoingPacketGobs[i].x, outgoingPacketGobs[i].y, 0, 0, true);
    }
}

static void UpdateGameObjects(void)
{
    for(int i=0; i<INCOMING_PACKET_GOB_COUNT; i++)
    {
        // Calculate next position of sprite.
        //testSprite.x += testSprite.dx;
        //testSprite.y += testSprite.dy;
        GobUpdate(&incomingPacketGobs[i]);

        // Hide if inside clouds
        if(incomingPacketGobs[i].y < CLOUD_SPRITE_Y || incomingPacketGobs[i].y>164)
            incomingPacketGobs[i].isHidden = true;
        else
            incomingPacketGobs[i].isHidden = false;

        GobDraw(&incomingPacketGobs[i]);
    }

    for(int i=0; i<OUTGOING_PACKET_GOB_COUNT; i++)
    {
        // Calculate next position of sprite.
        //testSprite.x += testSprite.dx;
        //testSprite.y += testSprite.dy;
        GobUpdate(&outgoingPacketGobs[i]);

        // Hide if inside clouds
        if(outgoingPacketGobs[i].y < CLOUD_SPRITE_Y || outgoingPacketGobs[i].y>164)
            outgoingPacketGobs[i].isHidden = true;
        else
            outgoingPacketGobs[i].isHidden = false;

         GobDraw(&outgoingPacketGobs[i]);
    }
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

static void DrawGame(void)
{
    // Draw top part with white (cloud)
    layer2_fill_rect(0, 0,  256, CLOUD_SPRITE_Y, 0xff, &off_screen);

    // Fill the lower screen area with black.
    layer2_fill_rect(0, CLOUD_SPRITE_Y, 256, (int)(194-CLOUD_SPRITE_Y), 123/*l.blue*/, &off_screen);

    //DrawPacket(&off_screen);

    DrawCloudEdge(&off_screen);

    //test_draw_text(&off_screen);

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
    
 
    DrawGame();
    DrawGame(); 


    while (true)
    {
        //DrawGame();

        // Wait for vertical blanking interval.
        intrinsic_halt();
        UpdateGameObjects();

        // char text[128];
        // sprintf(text, "y=%u", testSprite.y);
        // layer2_fill_rect(12*8, 3*8, 8*7, 8, 0xaa, &off_screen);
        // layer2_draw_text(3,  12, text, 0xEF, &off_screen);

        //printAt(23, 19); printf("yy = %d", testSprite.y); 

        // if (in_inkey())
        // {
        //     in_wait_nokey();
        //     select_test();
        // }

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

    // Trig a soft reset. The Next hardware registers and I/O ports will be reset by NextZXOS after a soft reset.
    //ZXN_NEXTREG(REG_RESET, RR_SOFT_RESET);
    //return 0;
}