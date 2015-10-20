cmake_minimum_required(VERSION 2.8)

include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if(COMPILER_SUPPORTS_CXX11)
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has C++11 support.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    message(STATUS "The compiler ${CMAKE_CXX_COMPILER} has C++0x support.")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qjson/src")
add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/qjson" "${CMAKE_CURRENT_BINARY_DIR}/3rdparty/qjson")

add_definitions(-DTHRUST_DEVICE_SYSTEM=THRUST_DEVICE_SYSTEM_OMP)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/modules")


find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_library(QGLVIEWER QGLViewer REQUIRED)

option(NOGUI OFF)

if (${NOGUI})
    find_package(Qt4 COMPONENTS QtCore REQUIRED)
    add_definitions(-DNOGUI)
    include(${QT_USE_FILE})
else ((${NOGUI}))
    find_package(Qt4 COMPONENTS QtGui QtCore QtOpenGL REQUIRED)
    set(QT_USE_QTOPENGL 1)
    include(${QT_USE_FILE})
    find_package(OpenGL REQUIRED)
    include_directories(${OpenGL_INCLUDE_DIRS})
    add_subdirectory(src/designer)
endif (${NOGUI})

add_subdirectory(src)

#testing stuff

option(WITH_TESTING OFF)

if (${WITH_TESTING})
    add_subdirectory(tests)
endif (${WITH_TESTING})

message(STATUS ${CMAKE_CXX_COMPILER_ID})
