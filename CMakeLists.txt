cmake_minimum_required(VERSION 3.20)
project(BarcodeRG CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

file(GLOB SOURCES CONFIGURE_DEPENDS
    *.cpp *.h *.ui *.qrc
    src/*.cpp src/*.h src/*.ui src/*.qrc
    ui/*.cpp ui/*.h ui/*.ui ui/*.qrc)

list(FILTER SOURCES EXCLUDE REGEX "test_roundtrip\\.cpp$")

add_executable(app ${SOURCES})

target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(app PRIVATE ${OpenCV_LIBS} Qt6::Widgets)

add_executable(roundtrip_test
    src/test_roundtrip.cpp
    src/code128.cpp
    src/barcode_render.cpp
    src/barcode_reader.cpp
    src/barcode_decode.cpp
    src/image_prep.cpp)

target_include_directories(roundtrip_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(roundtrip_test PRIVATE ${OpenCV_LIBS})

if(WIN32)
  find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
  if(WINDEPLOYQT)
    add_custom_command(TARGET app POST_BUILD
      COMMAND "${WINDEPLOYQT}" --no-translations "$<TARGET_FILE:app>")
  endif()
endif()