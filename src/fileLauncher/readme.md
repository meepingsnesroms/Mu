# Automatic file launcher for Mu

This along with an internal program allows booting .prc, .pqa, .zip and .img files, .img files can be passed with an optional .info file with any .img file to specify any attributes not stored in the flash memory.

There is also the option to load multiple files at the same time and specify the boot file.

In .zip files the file named "launch.prc" in the root of the zip is the boot file, if this file does not exist the zip is not booted, its contents are just passively installed.

The supported file types are:
* .prc(bootable)
* .pdb(not bootable)
* .pqa(bootable)
* .zip(bootable)
* .img(bootable)
* .info + .img(bootable)

All file types can be mixed, the last file loaded with the boot bit set will be the file that boots.

.img files can not be loaded with any other file types.
