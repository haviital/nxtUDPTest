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
    const uint8_t width = 62;
    const uint8_t height = 8;
    for(uint8_t r=0; r<height; r++)
        for(uint8_t c=0; c<width; c++)
            TextTileMapPutColorOnlyPos(row + r, col + c);

    // Draw top and bottom borders
    for(uint8_t c=0; c<width; c++)
    {
        TextTileMapPutcPos(row + 0, col + c, 131);
        TextTileMapPutcPos(row + height - 1, col + c, 140);
    }
    // Draw left and right borders
    for(uint8_t r=0; r<height; r++)
    {
        TextTileMapPutcPos(row + r, col + 0, 138);
        TextTileMapPutcPos(row + r, col + width - 1, 133);
    }

    // Draw the text
    TextTileMapPutsPos(15,10, "Give the server IP address? ");
    screencolour = TT_COLOR_IP_DIALOG_INFO_TEXT;
    TextTileMapPutsPos(17,11, "Press \"d\" to delete a character. ");
    TextTileMapPutsPos(18,11, "Add \"Space\" to fill the field if needed.");
    TextTileMapPutsPos(19,11, "Press \"Enter\" to accept and save to the config file.");
    TextTileMapPutsPos(20,11, "Delete the file NxtUdpTest.cfg to reset the address.");
}

void PrettyAddress(char* address, char* prettyAddr)
{
    uint8_t j=0;
    bool isDotZeroSequencePrefix = false;
    for(int i=0; i<=strlen(address); i++)
    {
        if(address[i]==' ')
            ; // skip spaces
        else
        {
            // Fix all ".0N" => ".N". (or "0N" at the start of a string).
            // Note that ".0." or "0" in the end of the string is accepted.
            if( isDotZeroSequencePrefix && 
                address[i] != '.' && address[i] != 0 ) 
                    j--; // Write over the previous zero.

            // Copy char to the real server address.
            prettyAddr[j++] = address[i];

            // Check for existing ".0" sequence.
            if(j>1 && prettyAddr[j-2] == '.' && serverAddress[j-1] == '0')
            {
                // Found ".0" sequence.
                isDotZeroSequencePrefix = true;
            }
            else if(j==1 && prettyAddr[j-1] == '0')
            {
                // Found "0" char at the start of the string.
                isDotZeroSequencePrefix = true;
            }
            else 
                isDotZeroSequencePrefix = false;
        }
    }
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

        // Draw the current ip address to the dialog.
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
        PrettyAddress(tmpIPAddress, serverAddress);

        // Print client and server IP address on the bottom.
        printIpAddressesOnUI();

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