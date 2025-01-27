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
    for(uint8_t r=0; r<6; r++)
        for(uint8_t c=0; c<62; c++)
            TextTileMapPutColorOnlyPos(row + r, col + c);

    TextTileMapPutsPos(15,10, "Give the server IP address? ");
    screencolour = TT_COLOR_IP_DIALOG_INFO_TEXT;
    TextTileMapPutsPos(17,10, "Press \"a\" to accept and save to the config file. ");
    TextTileMapPutsPos(18,10, "You can reset it by deleting the config file: NxtUdpTest.cfg");
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
        else if(key==12)  // del
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
        else if(key=='a')  // a = accept
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
    TextTileMapPutsPos(25, 21, localIpAddress);
    //TextTileMapPutsPos(25, 21, "123.456.78.9");  // For the screenshot
    // Print server ip address.
    screencolour = TT_COLOR_WHITE;
    TextTileMapPutsPos(25, 44, "               ");   
    TextTileMapPutsPos(25, 44, serverAddress);
    //TextTileMapPutsPos(25, 44, "987.654.32.1");  // For the screenshot
}