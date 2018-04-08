# Palm OS Hardware Test Suite

The tests available in the end will be:  
Dump Bootloader(Original, EZ, VZ)  
Rom Writes  
m68k CPU Version  
Test Emu Registers  
Verify TBLXX Opcodes  


## Warning
The version of gcc avalible for Palm OS is very old, any change you make must be c89 compatible or it wont compile!

If you enable unsafe mode you will have to reboot to exit, this is because it will take over the whole OS including interrupts, reset vectors(force crashes may be required for some tests), I/O ports, RAM control and clock speed.

In the hex viewer registers that are not safe to read will say "UNSAFE" instead of a hex value, the main unsafe reads are the uart registers.

There is almost no memory, 4KB stack on some devices.  
(Resolved, writing off the end of the framebuffer caused this)For some reason the official FileClose(FileHandle) crashes with "Fatal Exeption" on Clie Peg-SL10 and "illegal instruction 19F0 at 00000002" on Tungsten E and black screen reboot on Palm T|X, I think that it corrupts the stack triggering a jump to 0x00000000.
