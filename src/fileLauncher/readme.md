# Automatic file launcher for Mu

## This module is completely optional!!!
By default it is not compiled in with the emu, to build it in define EMU_HAVE_FILE_LAUNCHER.

This allows booting .prc, .pqa and .img files, .img files can be passed with an optional .info file to specify any attributes not stored in the flash memory.

There is also the option to load multiple files at the same time.

The supported file types are:
* .prc(bootable)
* .pdb(not bootable)
* .pqa(bootable)
* .img(bootable)
* .info + .img(bootable)

All file types can be mixed.
.img files can not be loaded with any other .img files.

The last runable application in the list passed to launcherLaunch() will be the file the emu boots.
