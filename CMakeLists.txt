cmake_minimum_required(VERSION 3.20)
project(BarcodeRG CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)

find_package(OpenCV REQUIRED)
find_package(ZXing CONFIG REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(app main.cpp)
target_link_libraries(app PRIVATE ${OpenCV_LIBS} ZXing::ZXing Qt6::Widgets)

if(WIN32)
  find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
  if(WINDEPLOYQT)
    add_custom_command(TARGET app POST_BUILD
      COMMAND "${WINDEPLOYQT}" --no-translations "$<TARGET_FILE:app>")
  endif()
endif()