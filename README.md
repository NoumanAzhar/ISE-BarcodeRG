# ISE-BarcodeRG
A C++ desktop application for generating and reading custom barcodes


# SETUP GUIDE
1. Clone the repo and switch to your branch and open VS
2. Install vcpkg
    Commands: 
    1. cd C:\
    2. git clone https://github.com/microsoft/vcpkg
    3. .\vcpkg\bootstrap-vcpkg.bat
3. setx VCPKG_ROOT "C:\vcpkg"
4. setx CMAKE_PREFIX_PATH "C:\Qt\6.12.0\msvc2022_64"
5. Restart Visual Studio    
6. Install Qt 6.12
     Install in C drive preferably 
     *Important*
     Tick custom installation and go to Qt for development -> Qt 6.12.0 -> tick MSVC 2022 64-bit and uncheck MinGW
7. Pull all the files from main
8. Select the default configure preset (switch from debugx64)
9. Build, the first build takes around 30 mins
10. Run to check the sample main.cpp   
