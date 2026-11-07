# ISE-BarcodeRG
A C++ desktop application for generating and reading custom barcodes


# SETUP GUIDE
1. Clone the repo and switch to your branch and open VS
2. Install vcpkg
    Commands: 
    1. cd C:\
    2. git clone https://github.com/microsoft/vcpkg
    3. .\vcpkg\bootstrap-vcpkg.bat    
3. Install Qt 6.12
     Install in C drive preferably 
     *Important*
     Tick custom installation and go to Qt for development -> Qt 6.12.0 -> tick MSVC 2022 64-bit and uncheck MinGW
4. setx VCPKG_ROOT "C:\vcpkg"
5. setx CMAKE_PREFIX_PATH "C:\Qt\6.12.0\msvc2022_64"
6. Restart Visual Studio
7. Select the default configure preset (switch from debugx64)
8. Build, the first build takes around 30 mins
9. Run to check the sample main.cpp   
