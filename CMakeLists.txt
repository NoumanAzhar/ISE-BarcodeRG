cmake_minimum_required(VERSION 3.20)
project(BarcodeRG CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

# backend: barcode + POS code, no Qt, shared by the app and the tests 
file(GLOB BACKEND_SOURCES CONFIGURE_DEPENDS
    Barcode/*.cpp Barcode/*.h
    POS/*.cpp POS/*.h)

# TEMP: Unfinished POS code, exclude it from the backend library for now
list(FILTER BACKEND_SOURCES EXCLUDE REGEX "/POS/")

add_library(backend STATIC ${BACKEND_SOURCES})
target_include_directories(backend PUBLIC
    ${CMAKE_SOURCE_DIR}/Barcode
    ${CMAKE_SOURCE_DIR}/POS)
target_link_libraries(backend PUBLIC ${OpenCV_LIBS})
target_compile_definitions(backend PUBLIC
    "DATA_DIR=\"${CMAKE_SOURCE_DIR}/Data\"")

# app.exe
file(GLOB UI_SOURCES CONFIGURE_DEPENDS
    UI/*.cpp UI/*.h UI/*.ui UI/*.qrc)

add_executable(app main.cpp ${UI_SOURCES})
target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}/UI
    ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(app PRIVATE backend Qt6::Widgets)

# tests: one program built from Testing/Test.cpp
add_executable(Test Testing/Test.cpp)
target_link_libraries(Test PRIVATE backend)

if(WIN32)
  find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
  if(WINDEPLOYQT)
    add_custom_command(TARGET app POST_BUILD
      COMMAND "${WINDEPLOYQT}" --no-translations "$<TARGET_FILE:app>")
  endif()
endif()