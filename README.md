# UDP test program

## Visible memory map (64k address space)

$6164 CODE
		- driver_terminal_input, driver_terminal_output, stdio
		- $7853-$91FE *** program code ***

$929F - $9D55 (size=$0AB6 (dec:2742))
	RODATA
		- e.g. $9405 _cloudSpr
		
$9E32 - $9F99
	DATA
	
$9F9D - $A493	
	BSS

$BE00 - $C000
	STACK (size=$200)

## Memory banks in global memory (16kb)
$0000 $3fff L2 buffer / rom
$4000 $7fff code ($6164) (or ULA screen?)
$8000 $bfff data($929f), stack ($c000), (free ram)
$c000 $ffff ????

