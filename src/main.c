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
#include <stdlib.h>

#include "lib/zxn/zxnext_layer2.h"
#include "lib/zxn/zxnext_sprite.h"
#include "defines.h"

#pragma output CRT_ORG_CODE = 0x6164
#pragma output REGISTER_SP = 0xC000
#pragma output CRT_STACK_SIZE = 0x400
#pragma output CLIB_MALLOC_HEAP_SIZE = 0
#pragma output CLIB_MALLOC_HEAP_SIZE = 0
#pragma output CLIB_STDIO_HEAP_SIZE = 0
#pragma output CLIB_FOPEN_MAX = -1

// #include "esp.h"
#include "uart2.h"
#include "GameObject.h"
#include "gfx.h"
#include "netcom.h"

/*
 * Define IDE_FRIENDLY in your C IDE to disable Z88DK C extensions and avoid
 * parser errors/warnings in the IDE. Do NOT define IDE_FRIENDLY when compiling
 * the code with Z88DK.
 */
#ifdef IDE_FRIENDLY
#define __z88dk_fastcall
#define __preserves_regs(...)
#endif

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

extern int TEST_main(void);

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

// Global

// NOTE: YOU NEED DEFINE THESE IN A SEPARATE HEADER: "mycredentials.h" !!!
//       THAT SHOULD BE NOT STORED TO GITHUB.
//char g_wifiSsid[64] = "myssid"; 
//char g_wifiPassword[128] = "mypassword";
#include "..\..\mycredentials.h"

// Defines
#define DATA_SPR_SPEED 5
#define PACKET_SPRITE_PATTERN_SLOT 0
#define CLOUD_SPRITE_PATTERN_SLOT 1
#define SERVER_SPRITE_0_PATTERN_SLOT 2
#define SERVER_SPRITE_1_PATTERN_SLOT 3
#define SERVER_SPRITE_2_PATTERN_SLOT 4
#define SERVER_SPRITE_3_PATTERN_SLOT 5
#define SPECNEXT_SPRITE_0_PATTERN_SLOT 6
#define SPECNEXT_SPRITE_1_PATTERN_SLOT 7


// Local

static uint8_t test_number = 0;

static layer2_screen_t shadow_screen = {SHADOW_SCREEN};

//layer2_screen_t off_screen = {OFF_SCREEN, 0, 1, 3};

#define INCOMING_PACKET_GOB_COUNT 20
static GameObject incomingPacketGobs[INCOMING_PACKET_GOB_COUNT];
#define OUTGOING_PACKET_GOB_COUNT 8
static GameObject outgoingPacketGobs[OUTGOING_PACKET_GOB_COUNT];
static uint8_t guardArr1[4];

enum state
{
    STATE_NONE = 0,
    STATE_CALL_NOP = 1,
    STATE_WAIT_FOR_NOP = 2
};

uint8_t gameState = STATE_NONE;
uint8_t frameCountForOneSecond = 0;
uint8_t frameCount8Bit = 0;
uint16_t totalSendPacketCount = 0;
uint16_t totalReceivedPacketCount = 0;
uint16_t sendPacketsPerSecondInterval = 0;
uint16_t recvPacketsPerSecondInterval = 0;
uint16_t sendPacketCountPerSecond = 0;
uint16_t recvPacketCountPerSecond = 0;
uint8_t numClonedPackets = 3;
uint16_t totalSeconds = 0;
 
void StartNewPacket(bool isIncoming);
void PageFlip(void);


/*******************************************************************************
 * Functions
 ******************************************************************************/

static void init_hardware(void)
{
    // Put Z80 in 28 MHz turbo mode.
    ZXN_NEXTREG(REG_TURBO_MODE, 0x03);

    // Disable RAM memory contention.
    ZXN_NEXTREGA(REG_PERIPHERAL_3, ZXN_READ_REG(REG_PERIPHERAL_3) | RP3_DISABLE_CONTENTION);

    //IO_153B = 0x00;  // Select ESP for UART(?)

    // Set UART to 115 kbaud
    //uart_set_prescaler(uart_compute_prescaler(115200UL));
    
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

static void create_sprites(void)
{
    // *** Static sprites

    #ifndef NO_GFX
    // Cloud sprite 0
    set_sprite_slot(CLOUD_SPRITE_PATTERN_SLOT);
    set_sprite_pattern(cloudSpr);
    set_sprite_slot(100);
    set_sprite_attributes_ext(CLOUD_SPRITE_PATTERN_SLOT, 30, 40, 0, 0, true);

    // Cloud sprite 1
    //set_sprite_slot(101);
    //set_sprite_pattern(cloudSpr);
    set_sprite_slot(101);
    set_sprite_attributes_ext(CLOUD_SPRITE_PATTERN_SLOT, 216, 40, 0, 0, true);

    // Server sprite 0
    set_sprite_slot(SERVER_SPRITE_0_PATTERN_SLOT);
    set_sprite_pattern(serverSpr0);
    set_sprite_slot(102);
    set_sprite_attributes_ext(SERVER_SPRITE_0_PATTERN_SLOT, 208, 154, 0, 0, true);

    // Server sprite 1
    set_sprite_slot(SERVER_SPRITE_1_PATTERN_SLOT);
    set_sprite_pattern(serverSpr1);
    set_sprite_slot(103);
    set_sprite_attributes_ext(SERVER_SPRITE_1_PATTERN_SLOT, (int)(208+16), 154, 0, 0, true);

    // Server sprite 2
    set_sprite_slot(SERVER_SPRITE_2_PATTERN_SLOT);
    set_sprite_pattern(serverSpr2);
    set_sprite_slot(104);
    set_sprite_attributes_ext(SERVER_SPRITE_2_PATTERN_SLOT, 208, (int)(154+16), 0, 0, true);

    // Server sprite 3
    set_sprite_slot(SERVER_SPRITE_3_PATTERN_SLOT);
    set_sprite_pattern(serverSpr3);
    set_sprite_slot(105);
    set_sprite_attributes_ext(SERVER_SPRITE_3_PATTERN_SLOT, (int)(208+16), (int)(154+16), 0, 0, true);

    // SpecNext sprite 0
    set_sprite_slot(SPECNEXT_SPRITE_0_PATTERN_SLOT);
    set_sprite_pattern(specnextSpr0);
    set_sprite_slot(106);
    set_sprite_attributes_ext(SPECNEXT_SPRITE_0_PATTERN_SLOT, 28, (int)(154+10), 0, 0, true);

    // SpecNext sprite 1
    set_sprite_slot(SPECNEXT_SPRITE_1_PATTERN_SLOT);
    set_sprite_pattern(specnextSpr1);
    set_sprite_slot(107);
    set_sprite_attributes_ext(SPECNEXT_SPRITE_1_PATTERN_SLOT, 28+16, (int)(154+10), 0, 0, true);

    #endif  // NO_GFX

   // *** Moving sprites

    // Map sprite bitmap(pattern) to the certain pattern slot. 
    set_sprite_slot(PACKET_SPRITE_PATTERN_SLOT);
    set_sprite_pattern(packet);

    // Incoming data packet gobs
    GameObject defaultGob = {.x = 30, .y= 88, .sx=0, .sy=DATA_SPR_SPEED, .spriteIndex=0, .spritePaletteOffset=0,
        .spritePatternIndex = PACKET_SPRITE_PATTERN_SLOT, .isHidden=true, .isActive=false};
    for(int i=0; i<INCOMING_PACKET_GOB_COUNT; i++)
    {
        int sprIndex = i;
        incomingPacketGobs[i] = defaultGob;
        incomingPacketGobs[i].y = i*32;
        incomingPacketGobs[i].spriteIndex = sprIndex;
        incomingPacketGobs[i].spritePaletteOffset = 0;
        //set_sprite_slot(sprIndex);
        //set_sprite_pattern(packet);
        set_sprite_slot(sprIndex);
        set_sprite_attributes_ext(incomingPacketGobs[i].spritePatternIndex, 
            incomingPacketGobs[i].x, incomingPacketGobs[i].y, incomingPacketGobs[i].spritePaletteOffset, 
            0, !incomingPacketGobs[i].isHidden);
    }

    // Outgoing data packet gobs
    GameObject defaultGob2 = {.x = 40, .y= 100, .sx=0, .sy=DATA_SPR_SPEED, .spriteIndex=0, .spritePaletteOffset=0, 
        .spritePatternIndex = PACKET_SPRITE_PATTERN_SLOT, .isHidden=true, .isActive=false};
    for(int i=0; i<OUTGOING_PACKET_GOB_COUNT; i++)
    {
        int sprIndex = i+INCOMING_PACKET_GOB_COUNT;
        outgoingPacketGobs[i] = defaultGob2;
        //incomingPacketGobs[i].y = i*32;
        outgoingPacketGobs[i].spriteIndex = sprIndex;
        outgoingPacketGobs[i].spritePaletteOffset = 1;
        set_sprite_slot(sprIndex);
        set_sprite_attributes_ext(outgoingPacketGobs[i].spritePatternIndex, 
            outgoingPacketGobs[i].x, outgoingPacketGobs[i].y, outgoingPacketGobs[i].spritePaletteOffset, 
            0, !outgoingPacketGobs[i].isHidden);
    }

}

static void DrawCloudEdge(layer2_screen_t *screen)
{
    #ifdef NO_GFX
    return;
    #endif

    // Draw tiled cloud
    int i=0;
    int x=0;
    for(i=0; i<16; i++ )
    {
        layer2_blit_transparent(x, CLOUD_SPRITE_Y,  cloud, 16, 26, screen); // top
        x+=16;
    }
}

static void DrawGameBackground(void)
{
    #ifdef NO_GFX
    return;
    #endif

    // Draw top part with white (cloud)
    layer2_fill_rect(0, 0,  256, CLOUD_SPRITE_Y, 0xff, &shadow_screen);

    //Fill the lower screen area with black.
    layer2_fill_rect(0, CLOUD_SPRITE_Y, 256, (int)(194-CLOUD_SPRITE_Y), 123/*l.blue*/, &shadow_screen);

    DrawCloudEdge(&shadow_screen);

     // Swap the double buffered screen.
    layer2_flip_main_shadow_screen();
}

static void create_start_screen(void)
{
    zx_border(INK_BLACK);
    zx_cls(BRIGHT | INK_BLACK | PAPER_WHITE);

    printAt(10, 0);

    // Init ESP.
    NetComInit();

    //printf("Press any key to start\n");
    //in_wait_key();

    //!!HV DrawStatusTextAndPageFlip("Ping server");

    gameState = STATE_CALL_NOP;

    //printf("Press any key to start\n");
    //in_wait_key();
}

void FlipBorderColor(bool reset)
{
    return;

    static uint8_t color = 0;
    if(reset) color=0;
    zx_border(color);
    color++;
}


void StartNewPacket(bool isIncoming)
{
    if(isIncoming)
    {
        // Find first inactive (free) gob.
        int i=0;
        for(; i<INCOMING_PACKET_GOB_COUNT; i++)
            if(!incomingPacketGobs[i].isActive)
                break;

        // If a free packet was found,  
        if(i<INCOMING_PACKET_GOB_COUNT)
        {
            GameObject* gobp = &incomingPacketGobs[i];
            gobp->isActive = true;
            gobp->x = 216;
            gobp->y = 154;
            gobp->sx = 0;
            gobp->sy = -DATA_SPR_SPEED;
        }
    }
    else
    {
        //printf("!!HV start new outgoing packet. frm=%u\n", frameCount8Bit);

        // Outgoing packet
        // Find first inactive (free) gob.
        int i=0;
        for(; i<OUTGOING_PACKET_GOB_COUNT; i++)
            if(!outgoingPacketGobs[i].isActive)
                break;

        // If a free packet was found,  
        if(i<OUTGOING_PACKET_GOB_COUNT)
        {
            //printf("!!HV free outgoing packet found!\n");

            GameObject* gobp = &outgoingPacketGobs[i];
            gobp->isActive = true;
            gobp->x = 40;
            gobp->y = 154;
            gobp->sx = 0;
            gobp->sy = -DATA_SPR_SPEED;
        }
    }
}

void DrawStatusText(char* text)
{
    #ifdef NO_GFX
    printf(text);
    printf("\n");
    return;
    #endif

    uint8_t len = strlen(text);
    //uint8_t startPosX = 128 - (len/2);
    layer2_fill_rect(1, 3*8, 255, 8, 0xff, &shadow_screen); // Clear field.
    layer2_draw_text(3, 0/*startPosX*/, text, 0xc, &shadow_screen);
}

void DrawStatusTextAndPageFlip(char* text)
{
    #ifdef NO_GFX
    printf(text);
    printf("\n");
    return;
    #endif

    DrawStatusText(text);

    //UpdateGameObjects();
    PageFlip();  

    DrawStatusText(text);
}

static void UpdateGameObjects(void)
{
    #ifdef NO_GFX
    return;
    #endif

    if(!CheckMemoryGuards()) PROG_FAILED;

    for(int i=0; i<INCOMING_PACKET_GOB_COUNT; i++)
    {
        // Calculate next position of sprite.
        GameObject* gob = &incomingPacketGobs[i];
        if(gob->isActive)
            GobUpdate(gob);

        // Hide if inside clouds
        if(gob->y < CLOUD_SPRITE_Y || gob->y > 164)
            gob->isHidden = true;
        else
            gob->isHidden = false;       

        // Afer moved from the server up to cloud, start moving from cloud down to Next.
        if(gob->sy < 0 && gob->y < 40 )
        {
            gob->x = 30;
            gob->sx = 0;
            gob->sy = DATA_SPR_SPEED;           
        }

        // When reached SpecNext => inactive
        if( gob->x==30 && // Going down to Next
            gob->y > 164) // Reached Next
        {
            gob->isActive = false;
            //gob->isHidden = true;
            gob->y = 190;
            gob->sy = 0;
        }

        // Note: draw also an inactive gob so that the sprite is set to invisible.
        GobDraw(gob);
    }

    if(!CheckMemoryGuards()) PROG_FAILED;

    for(int i=0; i<OUTGOING_PACKET_GOB_COUNT; i++)
    {
        // Calculate next position of sprite.
        GameObject* gob = &outgoingPacketGobs[i];
        if(gob->isActive)
        {
            GobUpdate(gob);
        }
        // Hide if inside clouds
        if(gob->y < CLOUD_SPRITE_Y || gob->y > 164)
            gob->isHidden = true;
        else
            gob->isHidden = false;       

        // After moved from the server up to cloud, start moving from cloud down to Next.
        if(gob->sy < 0 && gob->y < 40 )
        {
            gob->x = 206;
            gob->sx = 0;
            gob->sy = DATA_SPR_SPEED;           
        }

        // When reached SpecNext => inactive
        if( gob->x==206 && // Going down to Next
            gob->y > 164) // Reached Server
        {
            gob->isActive = false;
            gob->y = 190;
            gob->sy = 0;
            //gob->isHidden = true;
        }

        if(!CheckMemoryGuards()) PROG_FAILED;

        // Note: draw also an inactive gob so that the sprite is set to invisible.
        //if(gob->spritePaletteOffset!=0)
        {
            //PROG_FAILED;
            //zx_border(INK_BLUE);
            //printf("!!HV: UpdateGameObjects outgoing. frm=%u\n", frameCount8Bit);
            //zx_border(INK_GREEN);

            //printf("!!HV: UpdateGameObjects outgoing x=%u, y=%u, hid=%u, act=%u, soff=%u, frm=%u\n", 
            //        gob->x, gob->y,gob->isHidden , gob->isActive, gob->spritePaletteOffset,
            //        frameCount8Bit);
        }
       
        GobDraw(gob);
        //zx_border(INK_GREEN);
 
    }
}

void UpdateAndDrawAll(void)      
{
    // *** Update game objects
    UpdateGameObjects();

    // *** Receive data from server.
    uint16_t receivedPacketCount = 0;
    uint8_t err =  ReceiveMessage(MSG_ID_TESTLOOPBACK, &receivedPacketCount);
    if(receivedPacketCount>0)
    {
        StartNewPacket(true);
        gameState = STATE_CALL_NOP;  
    }

    // *** Send data to server.
    // Only send every 8th frame
    if((frameCount8Bit & 0x7) == 0)
    {
        err =  SendMessage(MSG_ID_TESTLOOPBACK);
        StartNewPacket(false);
    }
}

void PageFlip(void)
{
        // Wait for vertical blanking interval.
        intrinsic_halt();

        // Swap the double buffered screen.
        layer2_flip_main_shadow_screen();
}

int16_t GetUsedStack(void)
{
    uint8_t stackEnd = 0;
    return(0xc000   - (uint16_t)&stackEnd);
}

bool CheckMemoryGuards(void)
{
    return(
        guardArr1[0]==0xfe && guardArr1[1]==0xfe && guardArr1[2]==0xfe && guardArr1[3]==0xfe
    );
}

int main(void)
{

    //
    guardArr1[0] = 0xfe;
    guardArr1[1] = 0xfe;
    guardArr1[2] = 0xfe;
    guardArr1[3] = 0xfe;

    init_hardware();
    init_isr();

    create_sprites();
        
    set_sprite_layers_system(true, false, LAYER_PRIORITIES_S_L_U, false);

    DrawGameBackground();
    DrawGameBackground(); 
 
    create_start_screen();

    // Loop until the end of the game.
    uint8_t test=0;
    while (true)
    {   
        // Read a a key.
        if (in_key_pressed(IN_KEY_SCANCODE_1))
            numClonedPackets = 1;
        else if (in_key_pressed(IN_KEY_SCANCODE_2))
            numClonedPackets = 3;
        else if (in_key_pressed(IN_KEY_SCANCODE_3))
            numClonedPackets = 7;      

        // Print on ULA screen.
        layer2_fill_rect(0, 0, 256, 8, 0xE3, &shadow_screen); // make a hole
        printAt(0, 0);
        printf("Cloned packets: %u", numClonedPackets);

        UpdateAndDrawAll();
 
        frameCount8Bit++;

        // Calc frame and packet counts.
        if( frameCountForOneSecond++ >= 50)
        {
            sendPacketsPerSecondInterval =  sendPacketCountPerSecond;
            recvPacketsPerSecondInterval =  recvPacketCountPerSecond;

            frameCountForOneSecond = 0;
            sendPacketCountPerSecond = 0;
            recvPacketCountPerSecond = 0;

            totalSeconds++;
        }

        char text[128];
        text[0]=0;

        #ifndef NO_GFX
        layer2_fill_rect( 0, 192 - 8, 255, 8, 0x00, &shadow_screen); // Clear field.
        #endif
    
        char tmpStr[64];

        // frame count
        // strcpy(text, "frame:");
        // itoa(frameCountForOneSecond, tmpStr, 10);
        // strcat(text, tmpStr);

        //layer2_draw_text(23, 0, text, 0xc, &shadow_screen); 

         // Packets per second
        //strcat(text, " snd/rcv:");

        
        itoa(totalSeconds, tmpStr, 10);
        strcat(text, tmpStr);
        strcat(text, "s ");

        itoa(sendPacketsPerSecondInterval, tmpStr, 10);
        strcat(text, tmpStr);
        strcat(text, "/");
        itoa(recvPacketsPerSecondInterval, tmpStr, 10);
        strcat(text, tmpStr);
        strcat(text, " pkg/s ");

        // Send and received packets
        itoa(totalSendPacketCount,tmpStr,10);
        strcat(text, tmpStr);
        strcat(text, "/");
        itoa(totalReceivedPacketCount,tmpStr,10);
        strcat(text, tmpStr);
        strcat(text, " pkg ");

        //!!HV 
        // printAt(8,0); 
        // printf("sendPacketCountPerSecond = %u\n", sendPacketCountPerSecond);
        // printf("recvPacketCountPerSecond = %u\n", recvPacketCountPerSecond);
        // printf("frame                    = %u\n", frameCountForOneSecond);
        // printf("send/recv when frame=0: %u/%u\n", sendPacketsPerSecondInterval, recvPacketsPerSecondInterval);
        FlipBorderColor(false);

        #ifndef NO_GFX
        layer2_draw_text(23, 0, text, 0xc, &shadow_screen); 
        #else
        //Draw a hole for the whole screen,
        layer2_fill_rect(0, 0, 256, 192, 0xE3, &shadow_screen); // make a hole
        printStrAt(23,5, text);
        #endif

        //zx_border(INK_BLACK);

        PageFlip();   
    }

    // Trig a soft reset. The Next hardware registers and I/O ports will be reset by NextZXOS after a soft reset.
    //ZXN_NEXTREG(REG_RESET, RR_SOFT_RESET);
    //return 0;
}

void prog_failed(char* sourceFile, int32_t lineNum, uint8_t err)
{
   zx_border(2);  // Set red border first!

   // Make L2 screen transparent.
   layer2_fill_rect(0, 0,  256, 192, 0xE3, &shadow_screen); // make a hole
   layer2_flip_main_shadow_screen();

   //printf("uart_failed()\n");
   uint16_t stackUsage = GetUsedStack();
   char text[128];
   sprintf(text, "FAILED (err:%u) in file: %s (%lu)\n", err, sourceFile, lineNum);
   printf(text);
   printf("Stack usage after error: 0x%X bytes\n", stackUsage);
   for(;;);
}
