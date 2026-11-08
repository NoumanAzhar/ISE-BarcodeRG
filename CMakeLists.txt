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
    src/*.cpp src/*.h src/*.ui src/*.qrc)

add_executable(app ${SOURCES})

target_include_directories(app PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_BINARY_DIR})

target_link_libraries(app PRIVATE ${OpenCV_LIBS} Qt6::Widgets)

if(WIN32)
  find_program(WINDEPLOYQT windeployqt HINTS "${Qt6_DIR}/../../../bin")
  if(WINDEPLOYQT)
    add_custom_command(TARGET app POST_BUILD
      COMMAND "${WINDEPLOYQT}" --no-translations "$<TARGET_FILE:app>")
  endif()
endif()