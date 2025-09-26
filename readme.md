# A newer Palm OS emulator(newer-Mu)


## Why?
Because the og Mu code is a bit werid and buggy when compiling as a standalone app, in other word, the Mu emulator is a huge pile of Sh1t (No offense)

## What's new?
*   **Modernized C++ Standard:** The project now uses C++17, enabling modern language features.
*   **Code Formatting:** Integrated `.clang-format` for consistent code style.
*   **CMake Build System:** A new, unified CMake-based build system has been introduced for easier cross-platform compilation.
*   **Microsoft Fluent Icons:** Updated icons to a more modern, Fluent icons.

## Credits
[Firebird Emu](https://github.com/nspire-emus/firebird) (ARMv5TE Core)  
[uARM](https://dmitry.gr/?r=05.Projects&proj=07.%20Linux%20on%208bit) (PXA260 CPU Peripherals and reference CPU core)  
[Musashi v3.4](https://github.com/kstenerud/Musashi) (last version that builds outside of MAME)(68k Core)  
[blip_buf 1.1.0](https://github.com/TASVideos/BizHawk/tree/master/blip_buf) (Audio Resampler)  
https://www.iconarchive.com/show/crystal-clear-icons-by-everaldo/App-palm-icon.html (Desktop Icon)  
https://tango.freedesktop.org/Tango_Icon_Library (Palm Action Buttons, All UI buttons)  
https://www.iconfinder.com/icons/2317746/chip_computer_cpu_device_frequency_microchip_processor_icon (hwTestSuite Icon)  
https://www.flaticon.com/free-icon/cow_235371#term=cow&page=1&position=13 (muExpDriver Icon)  
https://findicons.com/icon/164302/cursor (Libretro Port Joystick Cursor)

## Building
#### For RetroArch
Make sure you have done all the steps here https://docs.libretro.com/ under "For Developers/Compilation" so you build environment works.  

    cd ./libretroBuildSystem
    make

#### For Qt (Recommended)
This project now uses CMake for a unified build system.

**Dependencies:**
*   A C++17 compliant compiler (e.g., GCC, Clang, MSVC)
*   CMake (version 3.10 or later)
*   Qt (version 5.11 or later)

**Build Steps:**
```bash
mkdir build
cd build
cmake ..
make
```
This will create the `Mu` executable inside the `build` directory.

#### For Qt (Legacy qmake)
You can still use the original qmake build system if you prefer.

Install Qt 5.11.1 and Qt Creator(optional) if not installed  
Open the .pro file in Qt Creator and click "Run" or build from command line

##### To build without Qt Creator(tested)

    cd ./qtBuildSystem/Mu
    qmake
    make

#### TestSuite for Palm OS
Install prc-tools from the below link(self compiled or prepackaged VM)  

    cd ./tools/palm/hwTestSuite
    ./make.sh

#### MuExpDriver for Palm OS
Install prc-tools from the below link(self compiled or prepackaged VM)  

    cd ./tools/palm/muExpansionDriver
    ./make.sh

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

