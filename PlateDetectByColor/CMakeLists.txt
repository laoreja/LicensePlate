cmake_minimum_required(VERSION 3.3)
project(PlateDetectByColor)

find_package( OpenCV 3)
include_directories( ${OpenCV_INCLUDE_DIRS} )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp ColorLocate.cpp ColorLocate.h PlateDetect.h)
add_executable(PlateDetectByColor ${SOURCE_FILES})
target_link_libraries( PlateDetectByColor ${OpenCV_LIBS} )