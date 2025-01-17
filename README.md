# (WIP! ) UDP test program

## A short explanation of what the program does

The program sends UDP packets to the UDP server (hosted in my PC). The packet is a special test packet which my UDP server understands and it sends the actual payload data back as many times as requested in the packet header data. This mimics the situation where there are several clients. The packets are send every 8th frame (about 6 times a second at 50 hz frame refresh). One UDP packet is received on every frame.

The graphics in the program are partly informing about the real usage and partly only for the viewing pleasure. E.g. for receiving, the (blue) packet sprite animation *is lauched* each time the real UDP packet is received (supposing there is a free sprite to do that). The animation duration has nothing to do with the UDP communication thought. The same is true with outgoing (to the server) data packets. 

So this program is not made to measure the peak performance of the UDP communication. It purpose is to find out if it would be technically possible to make a complex game over client-server UDP based, online multiplayer system. 

Note: Unfortunately this is not something you can try, as it requires the special UDP server to exist. I plan to release the server codes also in public later.

## Usage

There are 3 keys that affect to functionality:
- "1": The program sends one packet and the server sends it back
- "2": The program sends one packet and the server sends 3 packets back
- "3": The program sends one packet and the server sends 7 packets back

The numeric data in the bottom of the screen is as follows. 
The first row:
- The total UDP packets send to the server.
- The packets sent per second
- The bytes sent per second (each packet has a 19 byte header and a 32 byte payload data). 

The second row:
- The total UDP packets received from the server.
- The packets received per second
- The bytes received per second (each packet has a 2 byte header and a 32 byte payload data).  
- The number of packets the server sends back for each received packet (meaning how many pseudo clients there are in addition to my client)


## TODO, NOT UP-TO_DATE! Visible memory map (64k address space)

$6164 CODE
		- driver_terminal_input, driver_terminal_output, stdio
		- $7853-$91FE *** program code ***
		- $8328 GobDraw()

$929F - $9D55 (size=$0AB6 (dec:2742))
	RODATA
		- e.g. $9405 _cloudSpr
		
$9E32 - $9F99
	DATA
	
// $9F9D - $A493	
$A79E - $ABE0
	BSS
	- __BSS_head = $A79E
	- __BSS_END_head = $ABE0
	- _incomingPacketGobs             = $A7A2
	- _outgoingPacketGobs             = $A8B0
	- _buffer2                        = $A8DE
	- _buf_256                        = $AAA1


$B000 


$BE00 - $C000
	STACK (size=$200)
	- __register_sp                   = $C000

## Memory banks (16kb each)
$0000 $3fff L2 buffer / rom                        => rom bank / layer 2
$4000 $7fff code ($6164) (or ULA screen?)          => bank 5
$8000 $bfff data($929f), stack ($c000), (free ram) => bank 2
$c000 $ffff ????	                               => bank 0

# Features used

Below is a list of some of the features used in the program:

- Users two libraries for easy access to graphics: zxnext_Layer2 and zxnext_sprite
- Uses the tilemap layer for implementing texts. The font bimtpa data is copied from ROM to the area accessible by the tilemap.
- A IM2 interrupt handler in C which updates an own frame counter.
- Uses ESP8266 via UART
- Uses UDP communication (via ESP) with the server.
- Implementes a very simple UDP server in Python.

# Lessons learned
This is my first Spectrum or Spectrum Next program ever, so obiously there has been *really a lot* to learn.
Here are the list of new things I have learned as I remember:

1) Setup the development environment: Z88DK, VCCode, CSpect with integrated debugger (Z88DKSPECT.EXE for converting the map file for the debugger).
2) Using the ESP8266 dongle in PC for CSpect to enable networking. AT commands was a new to me also. TeraTerm was used to study AT commands before implementing them in the program.
3) NextSync for transfering program to Next HW for testing.
4) I used zxnext_Layer2 and zxnext_sprite libraries for gfx in the program.
5) Created a text tilemap layer for printing text efficiently over the L2 layer.
6) Making sprite texture data using Gimp, gfx2next.exe and my custom python script for converting *.nxi to C array.
7) Segmented memory, banks, stack and program start addresses, etc. This is still in process as I do not need much memory for this UPD test program, but in the future I will. 

And, of course, the beautiful Spectrum Next device itself, which I was lucky enough to be able to purchase for Christmas 2024! There is so much to learn still in many aspects of the device and the system SW. Also, I have only scratched the surface of the game gallery of Next and the original Spectrum :-) 

In the learning process, the Specturm Next discord community has been a priceless help. There are very professional, helpful and enthusiastic people, and without them this would have taken much much more than a month, from zero to a working program ! 

# Next steps

The UDP test program v1 is ready but I plan to do more:
- Add and check the sequence numbers so that it can be checked that packets are not lost. 
- Measure the time (raster lines) the UDP comminication takes. This gives a hint how many cycles are left for the game. 