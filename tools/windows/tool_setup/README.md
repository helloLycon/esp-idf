# ESP-IDF Tools Installer for Windows

This directory contains source files required to build the tools installer for Windows.

The installer is built using [Inno Setup](http://www.jrsoftware.org/isinfo.php). At the time of writing, the installer can be built with Inno Setup version 6.0.2.

The main source file of the installer is `idf_tools_setup.iss`. PascalScript code is split into multiple `*.iss.inc` files.

Some functionality of the installer depends on additional programs:

* [Inno Download Plugin](https://bitbucket.org/mitrich_k/inno-download-plugin) — used to download additional files during the installation.

* [7-zip](https://www.7-zip.org) — used to extract downloaded IDF archives.

* [cmdlinerunner](cmdlinerunner/cmdlinerunner.c) — a helper DLL used to run external command line programs from the installer, capture live console output, and get the exit code.

## Steps required to build the installer

* Build cmdlinerunner DLL.
  - On Linux/Mac, install mingw-w64 toolchain (`i686-w64-mingw32-gcc`). Then build the DLL using CMake:
    ```
    mkdir -p cmdlinerunner/build
    cd cmdlinerunner/build
    cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain-i686-w64-mingw32.cmake -DCMAKE_BUILD_TYPE=Release ..
    cmake --build .
    ```
    This will produce `cmdlinerunner.dll` in the build directory.
  - On Windows, it is possible to build using Visual Studio, with CMake support installed. By default, VS produces build artifacts in some hard to find directory. You can adjust this in CmakeSettings.json file generated by VS.

* Download 7zip.exe [("standalone console version")](https://www.7-zip.org/download.html) and put it into `unzip` directory (to get `unzip/7za.exe`).

* Download [idf_versions.txt](https://dl.espressif.com/dl/esp-idf/idf_versions.txt) and place it into the current directory. The installer will use it as a fallback, if it can not download idf_versions.txt at run time.

* Create the `dist` directory and populate it with the tools which should be bundled with the installer. At the moment the easiest way to obtain it is to use `install.sh`/`install.bat` in IDF, and then copy the contents of `$HOME/.espressif/dist` directory. If the directory is empty, the installer should still work, and the tools will be downloaded during the installation.

* Build the installer using Inno Setup Compiler: `ISCC.exe idf_tools_setup.iss`.

* Obtain the signing keys, then sign `Output/esp-idf-tools-setup-unsigned.exe`.
