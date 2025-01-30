#include "ui.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arch/zxn.h>
#include <arch/zxn/esxdos.h>
#include <input.h>
#include <z80.h>

#include <intrinsic.h>
#include "TextTileMap.h"

void DrawIpAddressDialog(uint8_t row, uint8_t col)
{
    // Draw dialog background
    screencolour = TT_COLOR_IP_DIALOG_MAIN_TEXT;
    for(uint8_t r=0; r<7; r++)
        for(uint8_t c=0; c<62; c++)
            TextTileMapPutColorOnlyPos(row + r, col + c);

    // Draw top and bottom borders
    for(uint8_t c=0; c<62; c++)
    {
        TextTileMapPutcPos(row + 0, col + c, 131);
        TextTileMapPutcPos(row + 6, col + c, 140);
    }
    // Draw left and right borders
    for(uint8_t r=0; r<7; r++)
    {
        TextTileMapPutcPos(row + r, col + 0, 138);
        TextTileMapPutcPos(row + r, col + 61, 133);
    }

    // Draw the text
    TextTileMapPutsPos(15,10, "Give the server IP address? ");
    screencolour = TT_COLOR_IP_DIALOG_INFO_TEXT;
    TextTileMapPutsPos(17,10, "Press \"d\" to delete a character. ");
    TextTileMapPutsPos(18,10, "Press \"Enter\" to accept and save to the config file.");
    TextTileMapPutsPos(19,10, "Delete the file NxtUdpTest.cfg to reset the address.");
}

void InputIPAddress(uint8_t row, uint8_t col)
{
    screenx = col;
    screeny = row;
    uint16_t startScreenx =  screenx;
    uint16_t startScreeny =  screeny;

    // Try to open an existing config file.
    #define maxIpAddressLen 15 
    uint8_t inputPos = 0;
    bool readyToReadInput = false;
    char tmpIPAddress[16];
    tmpIPAddress[0] = '\0';
    while(true/*&& inputPos <= maxIpAddressLen*/)
    {
        char key = 0;

        if(readyToReadInput)
        {
            in_wait_key();     
            key = in_inkey();
        }

        if((key >= '0' && key <= '9') || key==32)  // number or space
        {
            // Draw number char.
            // Add to the out array.
            tmpIPAddress[inputPos++] = key;
            tmpIPAddress[inputPos] = '\0';

            // If gone beyond the end,  stay in the last char in the last field.
            if(inputPos > 14)     
               inputPos--;

            // If between fields, print dot and step forward.
            if((inputPos+1)%4 == 0)
            {
                tmpIPAddress[inputPos++] = '.';      
                tmpIPAddress[inputPos] = '\0';
            }
        }
        else if(key=='d')  // del
        {
            if(inputPos!=0)
            {
                // Sep backwards.
                inputPos--;

                // If between fields, print dot and step backwards.
                if((inputPos+1)%4 == 0)
                    tmpIPAddress[inputPos--] = '.';    

                // Set ending null.  
                tmpIPAddress[inputPos] = '\0';
           }
        }
        else if(key==13)  // enter = accept
        {
            break;
        }

        // Draw the current ip address.
        screenx = startScreenx;
        screeny = startScreeny;
        uint8_t outputLen = strlen(tmpIPAddress);
        for(int i=0;i<15;i++)
        {
            char ch = 0;
            if(i<outputLen)
            {
                ch = tmpIPAddress[i];
            }
            else
            {
                if((i+1)%4 == 0)
                    ch='.';
                else
                    ch = ' ';
            }

            if(ch=='.')
                screencolour = TT_COLOR_IP_DIALOG_IP_ADDRESS_POINTS;
            else
                screencolour = TT_COLOR_IP_DIALOG_IP_ADDRESS_FIELD;
            
            // Draw the charcter.
            TextTileMapPutc(ch);
        }

        // Draw the cursor
        screencolour = TT_COLOR_IP_DIALOG_CURSOR;  // blue backround, grey text
        TextTileMapPutColorOnlyPos(screeny, startScreenx+inputPos);

        // Truncate the IP address and store it to the global variable.
        uint8_t j=0;
        for(int i=0; i<strlen(tmpIPAddress); i++)
        {
            
            if(tmpIPAddress[i]==' ')
                ; // skip
            else
                serverAddress[j++] = tmpIPAddress[i];
        }
        serverAddress[j] = 0;

        // Print client and server IP address.
        printIpAddressesOnUI();

        //!!HV TEST
        screencolour = TT_COLOR_GREEN;
        char test_txt[8];
        itoa(key, test_txt, 10);
        strcat(test_txt, "   ");
        TextTileMapPutsPos(0,0, test_txt);

        // Wait for vertical blanking interval.
        intrinsic_halt();

        // Wait until the key is no more pressed.
        in_wait_nokey();

        // Ready to read actual input after the first round.
        readyToReadInput = true;
    }
}

void printIpAddressesOnUI(void)
{
   // Print my ip address.
    screencolour = TT_COLOR_WHITE;
    TextTileMapPutsPos(25, 21, "               ");   
    #ifdef SCREENSHOT_MODE
    TextTileMapPutsPos(25, 21, "123.456.78.9");  // For the screenshot
    #else
    TextTileMapPutsPos(25, 21, localIpAddress);
    #endif
    // Print server ip address.
    screencolour = TT_COLOR_WHITE;
    TextTileMapPutsPos(25, 44, "               ");   
    #ifdef SCREENSHOT_MODE
    TextTileMapPutsPos(25, 44, "987.654.32.1");  // For the screenshot
    #else
    TextTileMapPutsPos(25, 44, serverAddress);
    #endif
}