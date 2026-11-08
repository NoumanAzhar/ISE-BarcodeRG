# ISE-BarcodeRG

A C++ desktop application for generating and reading custom barcodes.

Libraries: Qt 6.12 (UI), OpenCV (camera).

Increment 1 : generate a barcode from a 6-digit number, save it as an image, and read it back from both an image file as well as the laptop camera.

## Setup (one time)

1. Install Visual Studio with the "Desktop development with C++" workload.
2. Install vcpkg (PowerShell):
   ```
   cd C:\
   git clone https://github.com/microsoft/vcpkg
   .\vcpkg\bootstrap-vcpkg.bat
   setx VCPKG_ROOT "C:\vcpkg"
   ```
3. Install Qt 6.12 in C:\Qt (Qt Online Installer). Choose Custom installation,
   then Qt for Development > Qt 6.12.0, tick "MSVC 2022 64-bit", untick MinGW.
4. Set the Qt path (change it if Qt is installed elsewhere):
   ```
   setx CMAKE_PREFIX_PATH "C:\Qt\6.12.0\msvc2022_64"
   ```
5. Close and reopen Visual Studio.

## Get the code and build

1. Clone the repo, create your branch, and merge the latest main:
   ```
   git fetch
   git checkout your-name
   git merge origin/main
   ```
2. Open the folder in Visual Studio (File > Open > Folder).
3. In the top toolbar, switch the configuration from x64-Debug to "default".
4. Right-click CMakeLists.txt > Configure Cache. The first time takes about
   30+ minutes (it builds OpenCV). Keep the laptop plugged in.
5. Select app.exe as the startup item, then Build > Build All, then Ctrl+F5.

## Project structure

- Put .cpp, .h, .ui and .qrc files in the repo root or in `src/`.
  CMake picks them up automatically, so there is no need to edit CMakeLists.txt.
  Files in deeper folders (e.g. `src/ui/`) are NOT picked up.
- Keep each .ui file next to its .h/.cpp, with one owner per .ui file.
- Keep UI code (Qt) separate from barcode logic (plain C++ and OpenCV).

## Team workflow

- Never push to `main` directly. Work on your own branch and open a pull request.
- Merge `origin/main` into your branch regularly.

## Troubleshooting

- "Could not find Qt6": CMAKE_PREFIX_PATH is wrong, or Visual Studio was not
  restarted after setting it.
- "Qt6Widgetsd.dll not found": make sure you have the latest CMakeLists.txt
  from main, then Rebuild All.
- Strange configure errors: right-click CMakeLists.txt > Delete Cache and Reconfigure.
