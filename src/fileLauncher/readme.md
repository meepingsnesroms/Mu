# Automatic file launcher for Mu

## This module is completely optional!!!
By default it is not compiled in with the emu, to build it in define EMU_HAVE_FILE_LAUNCHER.

This along with an internal program allows booting .prc, .pqa, .zip and .img files, .img files can be passed with an optional .info file to specify any attributes not stored in the flash memory.

There is also the option to load multiple files at the same time and specify the boot file.

In .zip files the file named "launch.prc" in the root of the zip is launched if the zip is the boot file.

The supported file types are:
* .prc(bootable)
* .pdb(not bootable)
* .pqa(bootable)
* .zip(bootable)
* .img(bootable)
* .info + .img(bootable)

All file types can be mixed, except .img files.
.img files can not be loaded with any other file types or other .img files.

NOTE: Some Palm apps would name prc files with the ".pdb" extension or pdb files with the ".prc" extension, for this reason they are being treated the same in the boot code, attempting to boot an actual pdb will still not work.
