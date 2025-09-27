# A newer Palm OS emulator(newer-Mu)


## Why?
Because the og Mu code is a bit werid and buggy when compiling as a standalone app, in other word, the Mu emulator is a huge pile of Sh1t (No offense)

## What's new?
*   **Qt 6 Support:** The project has been updated to build and run with Qt 6.
*   **Microsoft Fluent Icons:** Updated icons to a more modern, Fluent icons.
*   **Cross-Platform Build System:** A new universal Makefile to build for both Qt and libretro.

## Credits
[Firebird Emu](https://github.com/nspire-emus/firebird) (ARMv5TE Core)  
[uARM](https://dmitry.gr/?r=05.Projects&proj=07.%20Linux%20on%208bit) (PXA260 CPU Peripherals and reference CPU core)  
[Musashi v3.4](https://github.com/kstenerud/Musashi) (last version that builds outside of MAME)(68k Core)  
[blip_buf 1.1.0](https://github.com/TASVideos/BizHawk/tree/master/blip_buf) (Audio Resampler)  
https://www.iconarchive.com/show/crystal-clear-icons-by-everaldo/App-palm-icon.html (Desktop Icon)  
https://fluenticons.co (Palm Action Buttons, All UI buttons)  
https://www.iconfinder.com/icons/2317746/chip_computer_cpu_device_frequency_microchip_processor_icon (hwTestSuite Icon)  
https://www.flaticon.com/free-icon/cow_235371#term=cow&page=1&position=13 (muExpDriver Icon)  
https://findicons.com/icon/164302/cursor (Libretro Port Joystick Cursor)

## Building

This guide provides instructions for compiling the project from source.

### For Debian-based Linux (e.g., Ubuntu)

**1. Install Dependencies:**

First, update your package list and install the necessary build tools and Qt 6 development libraries:

```bash
sudo apt update
sudo apt install build-essential git qt6-base-dev qt6-multimedia-dev libqt6svg6-dev
```

**2. Compile the Code:**

This project uses a universal Makefile. To build the Qt application (recommended), run:

```bash
make qt
```

The executable `Mu` will be created in the `qtBuildSystem/Mu` directory.

To build the libretro core for Linux:
```bash
make libretro PLATFORM=linux_x86_64
```

### Other Builds

#### TestSuite for Palm OS
Install prc-tools from the below link(self compiled or prepackaged VM)  

    cd ./tools/palm/hwTestSuite
    ./make.sh

#### MuExpDriver for Palm OS
Install prc-tools from the below link(self compiled or prepackaged VM)  

    cd ./tools/palm/muExpansionDriver
    ./make.sh

## Contributing

Contributions are welcome! Here are some ways you can help:

### Reporting Bugs

If you find a bug, please open an issue on the GitHub repository. Be sure to follow the `ISSUE_TEMPLATE.md` and provide as much detail as possible, including:

*   A clear and concise description of the bug.
*   Steps to reproduce the bug.
*   Your operating system, build type (Qt or libretro), and the version you are using.
*   The MD5 checksum of your ROM files.

### Suggesting Enhancements

If you have an idea for a new feature or an improvement to an existing one, you can also open an issue on GitHub. Please provide a clear description of your suggestion and why you think it would be a valuable addition.

### Pull Requests

If you would like to contribute code, you can do so through a pull request:

1.  Fork the repository.
2.  Create a new branch for your feature or bug fix (e.g., `git checkout -b feature/my-new-feature`).
3.  Make your changes and commit them with a clear and descriptive commit message.
4.  Push your changes to your fork (e.g., `git push origin feature/my-new-feature`).
5.  Create a pull request from your fork to the main repository.

### Where to Start

If you're looking for a place to start, you can check out the `roadmap.txt` file for a list of planned features. You can also look through the files in the `bugs` directory for a list of known issues that need to be addressed.

## Running
#### Files
palmos40-en-m500.rom: f50e4d5e4d98dc831f2c34a9107651eb (MD5)  
palmos41-en-m515.rom: 83cb1d1c76e568b916dc2e7c0bf669f6 (MD5)  
palmos52-en-t3.rom: de46ec84d9aabf655eabdf9b00a3845d (MD5)   
bootloader-dbvz.rom: 9da101cd2317830649a31f8fa46debec (MD5)  

#### For Qt
 1. Run once to create the directory tree
 2. Copy "palmos41-en-m515.rom" and "palmos52-en-t3.rom" to "~/Mu"
 3. (Optional)Copy "bootloader-en-m515.rom" to "~/Mu"
 4. Run emu and press start button

 #### For RetroArch
 1. Download "Palm OS(Mu)" from "Online Updater->Core Updater"
 2. Go back, select "Load Core", select "Palm OS(Mu)"
 3. Copy "palmos41-en-m515.rom" and "palmos52-en-t3.rom" to the RetroArch system directory
 4. (Optional)Copy "bootloader-en-m515.rom" to the RetroArch system directory
 5. Run a .prc or .img file

## Tools
[Prc-tools, Palm OS SDKs, pilrc, pilot-link](https://github.com/meepingsnesroms/prc-tools-remix)

## License
<a rel="license" href="http://creativecommons.org/licenses/by-nc/3.0/us/"><img alt="Creative Commons License" style="border-width:0" src="https://i.creativecommons.org/l/by-nc/3.0/us/88x31.png" /></a><br />This work is licensed under a <a rel="license" href="http://creativecommons.org/licenses/by-nc/3.0/us/">Creative Commons Attribution-NonCommercial 3.0 United States License</a>.
