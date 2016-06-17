Copy this folder to the same directory as SG2D, src/main.cpp has an example app. To build on windows install Microsoft Visual Studio and put the SDL2 folder in Program Files(x86)/Microsoft Visual Studio 14.0/VC/include and run build_files_windows/make.bat (or makenopriv.bat if you get an error). Make sure you have the directory to cl.exe in your path. It will produce an exe in release_windows.

on Linux you have to sudo apt-get install install libsdl2-2.0-0:i386 libsdl2-image-2.0-0:i386 libsdl2-net-2.0-0:i386 libsdl2-mixer-2.0-0:i386 libsdl2-ttf-2.0-0:i386

on OSX just create an xcode project and add SG2D, res, and frameworks folders to the project