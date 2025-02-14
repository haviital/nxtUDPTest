/*******************************************************************************
 *
 * A program for testing UDP communication with a server.
 *
 * Hannu Viitala, 2025
 * 
 * License : MIT License (attached)
 *
  ******************************************************************************/

#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include <input.h>
#include <z80.h>

#include <intrinsic.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <im2.h>           // im2 macros and functions

#ifdef ZXNEXT_EMULATOR_MODE_INCLUDES_1
// For server ip address and port, and packet token
// Comment out if you want to use the ip address from the cfg file.
// #include "..\..\mycredentials.h"
#endif

#include "lib/zxn/zxnext_layer2.h"
#include "lib/zxn/zxnext_sprite.h"
#include "defines.h"
#include "TextTileMap.h"

// Memory settings.
#pragma output CRT_ORG_CODE = 0x8184
#pragma output REGISTER_SP = 0xFF58
#pragma output CLIB_MALLOC_HEAP_SIZE = 0
#pragma output CLIB_STDIO_HEAP_SIZE = 0
#pragma output CLIB_FOPEN_MAX = -1

#include "uart2.h"
#include "GameObject.h"
#include "gfx.h"
#include "netcom.h"
#include "ui.h"

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

// The sprite info struct.
typedef struct sprite_info {
    uint8_t x; // X coordinate in pixels
    uint8_t y; // Y coordinate in pixels
    int8_t dx; // Horizontal movement in pixels
    int8_t dy; // Vertical movement in pixels
} sprite_info_t;

// The state of the game engine.
enum {
    STATE_NONE = 0,
    STATE_CALL_NOP = 1, 
    STATE_WAIT_FOR_NOP = 2,
    STATE_WAIT_FOR_CIPSEND_RESP = 3
};

/*******************************************************************************
 * Function Prototypes
 ******************************************************************************/

static void init_hardware(void);
static void init_isr(void);
void StartNewPacketAnim(bool isIncoming);
void PageFlip(void);
void ReadConfigFileOrAskServerIP(void);

/*******************************************************************************
 * Defines
 ******************************************************************************/

// Sprites.
#define DATA_SPR_SPEED 5
#define PACKET_SPRITE_PATTERN_SLOT 0
#define CLOUD_SPRITE_PATTERN_SLOT 1
#define SERVER_SPRITE_0_PATTERN_SLOT 2
#define SERVER_SPRITE_1_PATTERN_SLOT 3
#define SERVER_SPRITE_2_PATTERN_SLOT 4
#define SERVER_SPRITE_3_PATTERN_SLOT 5
#define SPECNEXT_SPRITE_0_PATTERN_SLOT 6
#define SPECNEXT_SPRITE_1_PATTERN_SLOT 7
#define CROSS_SPRITE_PATTERN_SLOT 8
#define OUTGOING_PACKET_X1 34
#define INCOMING_PACKET_X1 219
#define OUTGOING_PACKET_X2 214
#define INCOMING_PACKET_X2 29
#define INCOMING_PACKET_GOB_COUNT 20
#define OUTGOING_PACKET_GOB_COUNT 8

/*******************************************************************************
 * Variables
 ******************************************************************************/

// Tilemap colors.
uint8_t tilemap_background[16] = {
        0xE3,0x01,     // Transparent
        0xA0,0x00,     // Red
        0x14,0x00,     // Green
        0xAC,0x00,     // Yellow
        0x02,0x01,     // Blue
        0xA2,0x01,     // Magenta
        0x16,0x01,     // Cyan
        0xB6,0x01      // White
};
uint8_t tilemap_foreground[32] = {            // 0xE3 = 277
//      TRANS,     Red,       Green,     Yellow,    Blue,      Magenta,   Cyan,      White,
        0xE3,0x01, 0xA0,0x00, 0x14,0x00, 0xAC,0x00, 0x02,0x01, 0xA2,0x01, 0x16,0x01, 0xB6,0x01, // (normal)
        0x6D,0x01, 0xED,0x01, 0x7D,0x01, 0xFD,0x01, 0x77,0x01, 0xEE,0x01, 0x7F,0x01, 0xFF,0x01  // (bright)
};

static layer2_screen_t shadow_screen = {SHADOW_SCREEN};
static GameObject incomingPacketGobs[INCOMING_PACKET_GOB_COUNT];
static GameObject outgoingPacketGobs[OUTGOING_PACKET_GOB_COUNT];
char serverAddress[16];  // aaa.bbb.ccc.ddd
char serverPort[8];  // 1234567
char localIpAddress[16];
uint8_t gameState = STATE_NONE;
uint16_t engineFrameCount16t = 0;
uint16_t totalSendPacketCount = 0;
uint16_t totalReceivedPacketCount = 0;
uint16_t sendPacketsPerSecondInterval = 0;
uint16_t recvPacketsPerSecondInterval = 0;
uint16_t sendPacketCountPerSecond = 0;
uint16_t recvPacketCountPerSecond = 0;
uint8_t numClonedPackets = 3;
uint16_t totalSeconds = 0;
uint32_t recvRasterLineDur = 0;
uint8_t recvRasterLineFrames = 0;
uint32_t sendRasterLineDur = 0;
uint8_t sendRasterLineFrames = 0;
uint16_t oneSecondPassedAtFrame = 0;
uint16_t lastReceivedPacketAt = 0;

// Frame counter by an interrupt function.
uint16_t frames16t;


/*******************************************************************************
 * Functions
 ******************************************************************************/

static void init_hardware(void)
{
    // Put Z80 in 28 MHz turbo mode.
    ZXN_NEXTREG(REG_TURBO_MODE, 0x03);

    // Disable RAM memory contention.
    ZXN_NEXTREGA(REG_PERIPHERAL_3, ZXN_READ_REG(REG_PERIPHERAL_3) | RP3_DISABLE_CONTENTION);

    // Layer 2 banks.
    layer2_set_main_screen_ram_bank(8);
    layer2_set_shadow_screen_ram_bank(11);
}

void init_tilemap(void)
{
    // 0x6E (110) R/W =>  Tilemap Base Address
    //  bits 7-6 = Read back as zero, write values ignored
    //  bits 5-0 = MSB of address of the tilemap in Bank 5
    ZXN_NEXTREG(0x6e, 0x6C);                                    // tilemap base address is 0x6C00

    // 0x6F (111) R/W => Tile Definitions Base Address
    //  bits 7-6 = Read back as zero, write values ignored
    //  bits 5-0 = MSB of address of tile definitions in Bank 5
    ZXN_NEXTREG(0x6f, 0x5C);                                    // base address 0x5c00 (vis.chars(32+) at 0x5D00)

    // Copy font data from ROM to the tiledetails data.
    uint8_t* fontDataInRom = (void*)0x3D00;
    for(int i=0; i<(256-32); i++)
        memcpy(&(tiles[i].bmp), fontDataInRom+(8*i), 8);

    ZXN_NEXTREG(REG_GLOBAL_TRANSPARENCY_COLOR, 0xE3);
    ZXN_NEXTREG(REG_FALLBACK_COLOR, 0x00);

    // Select ULA palette
    ZXN_NEXTREG(0x43, 0x00);                                    // 0x43 (67) => Palette Control, 00 is ULA
    // Set Magenta back to proper E3
    ZXN_NEXTREGA(REG_PALETTE_INDEX, 27);                        // 0x40 (64) => Palette Index
    ZXN_NEXTREGA(REG_PALETTE_VALUE_8, 0xE3);                    // 0x41?

    ZXN_NEXTREG(REG_PALETTE_CONTROL, 0x30);                     // 0x43 (67) => Palette Control, 0x30 it tilemap first palette.
    ZXN_NEXTREG(REG_PALETTE_INDEX, 0);                          // 0x40 (64) => Palette Index
    uint8_t i = 0;
    do {
        //BG
        ZXN_NEXTREGA(0x44, tilemap_background[2*(i/32)]);       // 0x44 (68) => 9 bit colour) autoinc after TWO writes
        ZXN_NEXTREGA(0x44, tilemap_background[2*(i/32)+1]);     // 0x44 (68) => 9 bit colour) autoinc after TWO writes
        //FG
        ZXN_NEXTREGA(0x44, tilemap_foreground[(i%32)]);         // 0x44 (68) => 9 bit colour) autoinc after TWO writes
        ZXN_NEXTREGA(0x44, tilemap_foreground[(i%32)+1]);       // 0x44 (68) => 9 bit colour) autoinc after TWO writes
    } while ((i = i + 2) != 0);

    /*
     * 0x6B (107) => Tilemap Control
     * (R/W)
     *   bit 7 = 1 Enable the tilemap (soft reset = 0)
     *   bit 6 = 0 for 40x32, 1 for 80x32 (soft reset = 0)
     *   bit 5 = Eliminate the attribute entry in the tilemap (soft reset = 0)
     *   bit 4 = Palette select (soft reset = 0)
     *   bit 3 = Select textmode (soft reset = 0)
     *   bit 2 = Reserved, must be 0
     *   bit 1 = Activate 512 tile mode (soft reset = 0)
     *   bit 0 = Force tilemap on top of ULA (soft reset = 0)
     */
    ZXN_NEXTREG(0x6b, /*0b11001000*/ 0xC8);                     // enable tilemap, 80x32 mode, 1bit palette
    //ZXN_NEXTREG(0x6b,   /*0b10001000*/ 0x88);                     // enable tilemap, 40x32 mode, 1bit palette
    //ZXN_NEXTREG(0x6b, /*0b10001000*/ 0x08);                     // Disable tilemap, 40x32 mode, 1bit palette

    // bit 7    = 1 to disable ULA output
    // bit 6    = 0 to select the ULA colour for blending in SLU modes 6 & 7
    //          = 1 to select the ULA/tilemap mix for blending in SLU modes 6 & 7
    // bits 5-1 = Reserved must be 0
    // bit 0    = 1 to enable stencil mode when both the ULA and tilemap are enabled
    //             (if either are transparent the result is transparent otherwise the
    //              result is a logical AND of both colours)
    ZXN_NEXTREG(/*REG_ULA_CONTROL*/0x68, 0x80);  // Disable ULA screen. Use only the tilemap on U layer  
}

// There are a lot of ways to define an interrupt service routine
// This one is being done with a macro.  What makes a regular function
// different from an ISR is that the ISR must preserve cpu registers
// it is using and enable interrupts before returning with the special
// instruction "reti".  This macro creates a function called "isr" that
// does these things.  The 8080 in its name indicates it only saves
// the main registers af,bc,de,hl.  If you have code in there that modifies
// other registers (including c library code) you can use the IM2_DEFINE_ISR
// macro instead as that will save all of the z80's registers.  This
// is also a place where you can put ay music, etc.

IM2_DEFINE_ISR_8080(isr)
{
   // Update the 16 bit frame counter.
   ++frames16t;
}

static void init_isr(void)
{
    // Set up IM2 interrupt service routine:
    // Put Z80 in IM2 mode with a 257-byte interrupt vector table located
    // at 0x8000 (before CRT_ORG_CODE) filled with 0x81 bytes. Install an
    // empty interrupt service routine at the interrupt service routine
    // entry at address 0x8181 .

    intrinsic_di();
    im2_init((void *) 0x8000);
    memset((void *) 0x8000, 0x81, 257);

    z80_bpoke(0x8181, 0xc3);                // z80 JP instruction
    z80_wpoke(0x8182, (unsigned int)isr);   // to the isr routine

    intrinsic_ei();
}

void ReadConfigFileOrAskServerIP(void)
{
    // Try to open an existing config file.
    errno = 0;    
    uint8_t file = esx_f_open(PROGRAM_CONFIG_FILE, ESX_MODE_R|ESX_MODE_OPEN_EXIST);
    if (file!=0xff && !errno) 
    {
        // The file exists. Read the server IP.
        // Note: the address can be shorted than 16 characters.
        uint16_t len = esx_f_read(file, serverAddress, 16);
        if (len==0 || errno) 
        {
            int errno_org = errno;
            esx_f_close(file);
            PROG_FAILED1(errno_org);
        }
        serverAddress[len] = '\0';

        // Replace any CR or LF with '\0'.
        for(int8_t i=0; i<len; i++)
            if(serverAddress[i]=='\r' || serverAddress[i]=='\n')
                serverAddress[i] = '\0';

        esx_f_close(file);
    }
    else
    {
        // The file does not exist yet. Ask the user for a server IP and save it to a file.
    
        // Draw the dialog.
        DrawIpAddressDialog(14, 9);

        // Read IP address typed by the user.
        serverAddress[0] = '\0';
        InputIPAddress(15, 39);

        // Create the config file.
        errno = 0;    
        uint8_t newFile = esx_f_open(PROGRAM_CONFIG_FILE, ESX_MODE_W|ESX_MODE_OPEN_CREAT);
        if (newFile==0xff || newFile==0) 
        {
            esx_f_close(newFile);
            PROG_FAILED1(newFile);
        }
        if (errno) 
        {
            int errno_org = errno;
            esx_f_close(newFile);
            PROG_FAILED1(errno_org);
        }

        // Write the IP address.
        uint16_t len = strlen( serverAddress );
        esx_f_write( newFile, serverAddress, len );
        esx_f_close( newFile );
    }
}

static void create_sprites(void)
{
    // *** Static sprites

    // Cloud sprite 0
    set_sprite_slot(CLOUD_SPRITE_PATTERN_SLOT);
    set_sprite_pattern(cloudSpr);
    set_sprite_slot(100);
    set_sprite_attributes_ext(CLOUD_SPRITE_PATTERN_SLOT, 30, 40, 0, 0, true);

    // Cloud sprite 1
     set_sprite_slot(101);
    set_sprite_attributes_ext(CLOUD_SPRITE_PATTERN_SLOT, 214, 40, 0, 0, true);

    // Server sprite 0
    set_sprite_slot(SERVER_SPRITE_0_PATTERN_SLOT);
    set_sprite_pattern(serverSpr0);
    set_sprite_slot(102);
    set_sprite_attributes_ext(SERVER_SPRITE_0_PATTERN_SLOT, 210, 154, 0, 0, true);

    // Server sprite 1
    set_sprite_slot(SERVER_SPRITE_1_PATTERN_SLOT);
    set_sprite_pattern(serverSpr1);
    set_sprite_slot(103);
    set_sprite_attributes_ext(SERVER_SPRITE_1_PATTERN_SLOT, (int)(210+16), 154, 0, 0, true);

    // Server sprite 2
    set_sprite_slot(SERVER_SPRITE_2_PATTERN_SLOT);
    set_sprite_pattern(serverSpr2);
    set_sprite_slot(104);
    set_sprite_attributes_ext(SERVER_SPRITE_2_PATTERN_SLOT, 210, (int)(154+16), 0, 0, true);

    // Server sprite 3
    set_sprite_slot(SERVER_SPRITE_3_PATTERN_SLOT);
    set_sprite_pattern(serverSpr3);
    set_sprite_slot(105);
    set_sprite_attributes_ext(SERVER_SPRITE_3_PATTERN_SLOT, (int)(210+16), (int)(154+16), 0, 0, true);

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

    // Cross sprite
    set_sprite_slot(CROSS_SPRITE_PATTERN_SLOT);
    set_sprite_pattern(crossSpr);
    set_sprite_slot(108);
    set_sprite_attributes_ext(CROSS_SPRITE_PATTERN_SLOT, (uint8_t)(210+4), (uint8_t)(154+4), 0, 0, false);

     // *** Define moving sprites (used by GameObjects)

    // Map sprite bitmap(pattern) to the certain pattern slot. 
    set_sprite_slot(PACKET_SPRITE_PATTERN_SLOT);
    set_sprite_pattern(packet);

    // Outgoing data packet gobs
    GameObject defaultGob2 = {.x = 40, .y= 100, .sx=0, .sy=DATA_SPR_SPEED, .spriteIndex=0, .spritePaletteOffset=0, 
        .spritePatternIndex = PACKET_SPRITE_PATTERN_SLOT, .isHidden=true, .isActive=false};
    for(int i=0; i<OUTGOING_PACKET_GOB_COUNT; i++)
    {
        int sprIndex = i;
        outgoingPacketGobs[i] = defaultGob2;
        outgoingPacketGobs[i].spriteIndex = sprIndex;
        outgoingPacketGobs[i].spritePaletteOffset = 1;
        set_sprite_slot(sprIndex);
        set_sprite_attributes_ext(outgoingPacketGobs[i].spritePatternIndex, 
            outgoingPacketGobs[i].x, outgoingPacketGobs[i].y, outgoingPacketGobs[i].spritePaletteOffset, 
            0, !outgoingPacketGobs[i].isHidden);
    }

    // Incoming data packet gobs
    GameObject defaultGob = {.x = 30, .y= 88, .sx=0, .sy=DATA_SPR_SPEED, .spriteIndex=0, .spritePaletteOffset=0,
        .spritePatternIndex = PACKET_SPRITE_PATTERN_SLOT, .isHidden=true, .isActive=false};
    for(int i=0; i<INCOMING_PACKET_GOB_COUNT; i++)
    {
        int sprIndex = i+OUTGOING_PACKET_GOB_COUNT;
        incomingPacketGobs[i] = defaultGob;
        incomingPacketGobs[i].y = i*32;
        incomingPacketGobs[i].spriteIndex = sprIndex;
        incomingPacketGobs[i].spritePaletteOffset = 0;
        set_sprite_slot(sprIndex);
        set_sprite_attributes_ext(incomingPacketGobs[i].spritePatternIndex, 
            incomingPacketGobs[i].x, incomingPacketGobs[i].y, incomingPacketGobs[i].spritePaletteOffset, 
            0, !incomingPacketGobs[i].isHidden);
    }
}

static void DrawGameBackground(void)
{
    // Draw top part with white (cloud)
    layer2_fill_rect(0, 0,  256, CLOUD_SPRITE_Y, 0xff, &shadow_screen);

    // Fill the lower screen area with black.
    layer2_fill_rect(0, CLOUD_SPRITE_Y, 256, (int)(194-CLOUD_SPRITE_Y), 123/*l.blue*/, &shadow_screen);

    // Draw the cloud edge.
    int x=0;
    for(int i=0; i<16; i++ )
    {
        layer2_blit_transparent(x, CLOUD_SPRITE_Y,  cloud, 16, 26, &shadow_screen);
        x+=16;
    }

    // Swap the double buffered screen.
    layer2_flip_main_shadow_screen();
}

// Launch a new packet animation, incoming or outgoing.
void StartNewPacketAnim(bool isIncoming)
{
    if(isIncoming)
    {
        // *** Incoming packet

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
            gobp->x = INCOMING_PACKET_X1;
            gobp->y = 154;
            gobp->sx = 0;
            gobp->sy = -DATA_SPR_SPEED;
        }
    }
    else
    {
        // *** Outgoing packet

        // Find first inactive (free) gob.
        int i=0;
        for(; i<OUTGOING_PACKET_GOB_COUNT; i++)
            if(!outgoingPacketGobs[i].isActive)
                break;

        // If a free packet was found,  
        if(i<OUTGOING_PACKET_GOB_COUNT)
        {
            GameObject* gobp = &outgoingPacketGobs[i];
            gobp->isActive = true;
            gobp->x = OUTGOING_PACKET_X1;
            gobp->y = 154;
            gobp->sx = 0;
            gobp->sy = -DATA_SPR_SPEED;
        }
    }
}

void DrawStatusText(char* text)
{
    layer2_fill_rect(1, 3*8, 255, 8, 0xff, &shadow_screen); // Clear field.
    layer2_draw_text(3, 0/*startPosX*/, text, 0xc, &shadow_screen);
}

void DrawStatusTextAndPageFlip(char* text)
{
    DrawStatusText(text);

    PageFlip();  

    DrawStatusText(text);
}


// Updates all moving game objects on the screen.
static void UpdateGameObjects(void)
{
   

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
            gob->x = INCOMING_PACKET_X2;
            gob->sx = 0;
            gob->sy = DATA_SPR_SPEED;           
        }

        // When reached SpecNext => inactive
        if( gob->x==INCOMING_PACKET_X2 && // Going down to Next
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
            gob->x = OUTGOING_PACKET_X2;
            gob->sx = 0;
            gob->sy = DATA_SPR_SPEED;           
        }

        // When reached SpecNext => inactive
        if( gob->x==OUTGOING_PACKET_X2 && // Going down to Next
            gob->y > 164) // Reached Server
        {
            gob->isActive = false;
            gob->y = 190;
            gob->sy = 0;
            //gob->isHidden = true;
        }

        // Draw gob.
        GobDraw(gob);
    }
}

void UpdateAndDrawAll(void)      
{
    // *** Update game objects
    UpdateGameObjects();

    // *** Update the cross sprite
    // Show it if there has not been packets from server for a second.
    bool isCrossVisible = frames16t > (lastReceivedPacketAt + 50);
    set_sprite_slot(108);
    set_sprite_attributes_ext(CROSS_SPRITE_PATTERN_SLOT, (uint8_t)(210+4), (uint8_t)(154+4), 0, 0, isCrossVisible);

    // A state machine.
    // Run until the state has not changed or until prevGameState is STATE_NONE.
    uint8_t prevGameState = 255;
    //while(gameState != prevGameState && prevGameState != STATE_NONE)
    bool exitLoop = false;
    while(!exitLoop)
    {
        prevGameState = gameState;

        switch(gameState)
        {
            case STATE_NONE:
            {
                // *** Receive data from server.
                bool hasReceivedPacket = false;
                uint8_t err =  NetComReceiveSendResponseOrDataPacketIfAny(&hasReceivedPacket);

                //CSPECT_BREAK_IF(recvRasterLineDiff>100);

                if(hasReceivedPacket)
                    StartNewPacketAnim(true);
 
                // *** Prepare sending data to server.
                // Only send every 8th frame
                if(gameState == STATE_NONE && (frames16t & 0x7) == 0)
                {
                    err = NetComPrepareSendMessage();
                    gameState = STATE_WAIT_FOR_CIPSEND_RESP;
                }
                else
                {
                    // Always exit the loop.
                    exitLoop = true;
                }
            }
            break;

            case STATE_WAIT_FOR_CIPSEND_RESP:
            {
                uint8_t netComCmd = NETCOM_CMD_NONE;
                if( NetComReceiveSendPromptOrDataCmdIfAny(/*OUT*/ &netComCmd) )
                    PROG_FAILED;   

                // Wait for the CIPSEND (sending data packet) response.
                if(netComCmd == NETCOM_CMD_CIPSEND_RESPONSE)
                {
                    // The CIPSEND cmd response received. Ready for sending the data. 

                    // Send the actual data packet to server.
                    NetComSendMessage();
                    gameState = STATE_NONE;

                    StartNewPacketAnim(false);
                }
                else if(netComCmd == NETCOM_CMD_IPD_RECEIVED)
                {
                    uint8_t err =  NetComReceiveAndCheckIPDDataPacket();

                    //CSPECT_BREAK_IF(recvRasterLineDiff>100);

                    if(!err)
                        StartNewPacketAnim(true);
                }
                
                // Always exit the loop.
                exitLoop = true;
            }
            break;

        }  // end switch

    }  // end while
}

void PageFlip(void)
{
        // Wait for vertical blanking interval.
        intrinsic_halt();

        // Swap the double buffered screen.
        layer2_flip_main_shadow_screen();
}

// Calculates the current stack usage.
int16_t GetUsedStack(void)
{
    uint8_t stackEnd = 0;
    return(0xFF58 - (uint16_t)&stackEnd);
}

void prog_failed(char* sourceFile, int32_t lineNum, uint8_t err)
{
    // Enable ULA screen.
    ZXN_NEXTREG(/*REG_ULA_CONTROL*/0x68, 0x00);  // Enable ULA screen. 

    //Disable the tilemap
    ZXN_NEXTREG(0x6b, /*0b10001000*/ 0x08); 

    // Set ULA the topmost layer
    set_sprite_layers_system(true, false, LAYER_PRIORITIES_U_S_L, false);

    zx_border(2);  // Set red border first!

    uint16_t stackUsage = GetUsedStack();
    char text[128];
    sprintf(text, "FAILED (err:%u) in file: %s (%lu)\n", err, sourceFile, lineNum);
    printf(text);
    printf("Stack usage after error: 0x%X bytes\n", stackUsage);

    // Print the log buffer.
    printf("UART LOG: \n");
    printPaper(PAPER_BLACK);
    printf("\x10\x36"); // Set BRIGHT INK to YELLOW = (char)0x10 and  (char)(0x30 + 0x06)
    print_log_buffer();

    for(;;); // Wait forever.
}

void main(void)
{
    // Use some default values when testing under the emulator. To use your own you need to define 
    // ZXNEXT_EMULATOR_MODE_INCLUDES in your environment variables and the defines in the
    // file: "..\..\mycredentials.h". For example:
    //    //#define UDP_SERVER_PORT "4444"
    //    #define UDP_SERVER_ADDRESS "123.456.789.123" 
    strcpy(serverPort, "4444");  // Use the fixed port for now for simplicity.
    #ifdef UDP_SERVER_ADDRESS
    // Copy the default values.
    strcpy(serverAddress, UDP_SERVER_ADDRESS);
    //strcpy(serverPort, UDP_SERVER_PORT);
    #endif

    // Initialize

    init_hardware();
    init_tilemap();
    init_isr();

    create_sprites();
    set_sprite_layers_system(true, false, LAYER_PRIORITIES_S_U_L, false);

    DrawGameBackground();
    DrawGameBackground(); // The second time for double buffering.
 
    zx_border(INK_BLACK);
    zx_cls(BRIGHT | INK_BLACK | PAPER_WHITE);

    NetComInit(); // Init ESP.
    
    gameState = STATE_NONE;
    
    // Draw the title and the bottom status area.

    const char* titleText = " >>> UDP TEST PROGRAM v1.1 <<<";
    layer2_draw_text(0, 0, titleText, 0x70, &shadow_screen); 
     // Clear bottom area.
    layer2_fill_rect( 0, (uint8_t)(192 - 16), 256, 16, 0x00, &shadow_screen); 
    // Swap the double buffered screen.
    layer2_flip_main_shadow_screen();    
    // Draw title for the other buffer. 
    layer2_draw_text(0, 0, titleText, 0x70, &shadow_screen); 
     // Clear bottom area for the other buffer.
    layer2_fill_rect( 0, (uint8_t)(192 - 16), 256, 16, 0x00, &shadow_screen); 
 
    // Clear the text tilemap
    TextTileMapClear();

    #ifndef UDP_SERVER_ADDRESS
    // Get the server IP address.
    ReadConfigFileOrAskServerIP();
    #endif

    // Connect to server.
    NetComConnectToServer(); 

    // Clear the text tilemap
    TextTileMapClear();

    // Print client and server IP address.
    printIpAddressesOnUI();

    // Loop until the end of the game.
    while (true)
    {   
        #ifdef DEBUG_TEXT_ENABLED
        screencolour = TT_COLOR_CYAN;
        TextTileMapGoto(10,0);
        #endif

       char text[128];
 
        // Read a key to change the number of packets the server sends.
        if (in_key_pressed(IN_KEY_SCANCODE_1))
            numClonedPackets = 1;
        else if (in_key_pressed(IN_KEY_SCANCODE_2))
            numClonedPackets = 3;
        else if (in_key_pressed(IN_KEY_SCANCODE_3))
            numClonedPackets = 7;      

        // TEST Show the fail view (for seeing the ESP log).
        //else if (in_key_pressed(IN_KEY_SCANCODE_4))
        //    PROG_FAILED;     

        // Draw the animation and handle the UDP packet receiving and sending.
        UpdateAndDrawAll();
 
        // Calc frame and packet counts in one second periods.
        if( frames16t == oneSecondPassedAtFrame)
        {
            sendPacketsPerSecondInterval =  sendPacketCountPerSecond;
            recvPacketsPerSecondInterval =  recvPacketCountPerSecond;
        }

        // Reset one second counters.
        if(frames16t >= oneSecondPassedAtFrame)
        {
            sendPacketCountPerSecond = 0;
            recvPacketCountPerSecond = 0;            
            totalSeconds++;

            // Set next update.
            oneSecondPassedAtFrame = frames16t + VIDEO_SCREEN_REFRESH_RATE;
        }

       // Print client and server send speed and send count.
        text[0]=0;
        char tmpStr[64];
        if((frames16t & 0x1f) == 0x1f ) // Every 32nd frame
        {
            // **** SEND INFO

            // Packet count
            screencolour = TT_COLOR_CYAN;
            strcpy(text, "Send: ");
            itoa(totalSendPacketCount, tmpStr, 10);
            strcat(text, tmpStr); 
            strcat(text, " pkg");
            TextTileMapPutsPos(26, 0, text);

            // Packets per second
            uint32_t sendPacketsPerSecond = sendPacketsPerSecondInterval;
            ltoa(sendPacketsPerSecond, tmpStr, 10);
            strcpy(text, tmpStr);
            strcat(text, " pkg/s ");
            TextTileMapPutsPos(26, 30, text);

            // Bytes per second
            uint32_t sendBytesPerSecond = sendPacketsPerSecondInterval * MSG_TESTLOOPBACK_REQUEST_STRUCT_SIZE;
            ltoa(sendBytesPerSecond, tmpStr, 10);
            strcpy(text, tmpStr);
            strcat(text, " B/s");
            TextTileMapPutsPos(26, 60, text);
        }

        if(((frames16t + 16) & 0x1f) == 0x1f ) // Every 32nd frame, starting from frame 16.
        {
            // **** RECEIVE INFO

            // Packet count
            screencolour = TT_COLOR_BLUE;
            strcpy(text, "Recv: ");
            itoa(totalReceivedPacketCount, tmpStr, 10);
            strcat(text, tmpStr);
            strcat(text, " pkg");
            TextTileMapPutsPos(27, 0, text);

            // Packets per second
            uint32_t recvPacketsPerSecond = recvPacketsPerSecondInterval;
            ltoa(recvPacketsPerSecond, tmpStr, 10);
            strcpy(text, tmpStr);
            strcat(text, " pkg/s ");
            TextTileMapPutsPos(27, 30, text);

            // Bytes per second
            uint32_t recvBytesPerSecond = recvPacketsPerSecondInterval * MSG_TESTLOOPBACK_RESPONSE_STRUCT_SIZE;
            ltoa(recvBytesPerSecond, tmpStr, 10);
            strcpy(text, tmpStr);
            strcat(text, " B/s");
            TextTileMapPutsPos(27, 60, text);

           // Print cloned count.
            strcpy(text, "x");
            itoa(numClonedPackets, tmpStr, 10);
            strcat(text, tmpStr);
            //layer2_draw_text(23, 27, text, 0x03, &shadow_screen);
            TextTileMapPutsPos(27, 75, text);
        }

        engineFrameCount16t++;

        // Flip the layer 2 page.
        PageFlip();   
    }
}
