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

