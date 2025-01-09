# UDP test program

## Visible memory map (64k address space)

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

# Lessons learned
This is my first Spectrum or Spectrum Next program ever, so obiously there has been really *a lot* to learn.
Here are the list of new things I have learned as I remember:

- The development environment: Z88DK, VCCode, CSpect with integrated debugger (Z88DKSPECT.EXE for converting the map file for the debugger).
- ESP8266 dongle in PC for CSpect to enable networking. AT commands was a new to me also. TeraTerm was used to study AT commands before implementing them in the program.
- NextSync for transfering program to Next HW for testing.
- I used zxnext_Layer2 and zxnext_sprite libraries for gfx in the program.
- The text tilemap for printing text efficiently.
- Making sprite texture data using Gimp and ???

And, of course, the beautiful Spectrum Next device itself, which I was lucky enough to be able to purchase for Christmas 2024! There is so much to learn still in many aspects of the device and the system SW. Also, I have only scratched the surface of the game gallery of Next and the original Spectrum :-) 

- Z88DK environment for developing C programs. In addition to the Z88DK the evironmant consists of the VSCode editor, CSpect emulator and debugger. It took a lot of time to setup and fine tune, but now it works very well! The CSpect emulator is just marvellous. The emulator is accurate enough that my program worked almost exactly the same on HW. The integrated debugger is very usable also, after I started to use Z88SPECT.EXE for converting the map file.  
- I used zxnext_Layer2 and zxnext_sprite libraries for graphics. That makes it easier to start making first programs.
- 
