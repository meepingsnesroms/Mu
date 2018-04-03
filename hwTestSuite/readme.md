# Palm OS Hardware Test Suite

The tests available in the end will be:  
Dump Bootloader(Original, EZ, VZ)  
Rom Writes  
m68k CPU Version  
Test emu registers  



## Warning
The version of gcc avalible for Palm OS is very old, any change you make must be c89 compatible or it wont compile!

If you enable unsafe mode you will have to reboot to exit, this is because it will take over the whole OS including interrupts, reset vectors(force crashes may be required for some tests), I/O ports, RAM control and clock speed.

In the hex viewer registers that are not safe to read will say "UNSAFE" instead of a hex value, the main unsafe reads are the uart registers.

There is almost no memory, 4KB stack on some devices.
