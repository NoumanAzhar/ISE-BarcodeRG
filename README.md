# ISE-BarcodeRG

A C++ desktop application for generating and reading custom barcodes.

Libraries: Qt 6.12 (UI), OpenCV (camera).

Increment 1 : generate a barcode from a 6-digit number, save it as an image, and read it back from both an image file as well as the laptop camera.

## Setup (one time)

1. Clone the repo and switch to your branch and open VS

2. Install vcpkg Commands:

   1. cd C:\

   2. git clone https://github.com/microsoft/vcpkg

   3. .\vcpkg\bootstrap-vcpkg.bat

3. Install Qt 6.12 Install in C drive preferably Important Tick custom installation and go to Qt for development → Qt 6.12.0 → tick MSVC 2022 64-bit and uncheck MinGW

4. setx VCPKG_ROOT "C:\vcpkg"

5. setx CMAKE_PREFIX_PATH "C:\Qt\6.12.0\msvc2022_64"

6. Restart Visual Studio

7. Select the default configure preset (switch from debugx64)

8. Build, the first build takes around 30 mins

9. Run to check the sample main.cpp

## Project structure

- Put .cpp, .h, .ui and .qrc files in the repo root or in src/.
  CMake picks them up automatically, so there is no need to edit CMakeLists.txt.
  Files in deeper folders (e.g. src/ui/) are NOT picked up.
- Keep each .ui file next to its .h/.cpp, with one owner per .ui file.
- Keep UI code (Qt) separate from barcode logic (plain C++ and OpenCV).

## Team workflow

- Never push to main directly. Work on your own branch and open a pull request.
- Merge origin/main into your branch regularly.

## Troubleshooting

- "Could not find Qt6": CMAKE_PREFIX_PATH is wrong, or Visual Studio was not
  restarted after setting it.
- "Qt6Widgetsd.dll not found": make sure you have the latest CMakeLists.txt
  from main, then Rebuild All.
- Strange configure errors: right-click CMakeLists.txt > Delete Cache and Reconfigure.
