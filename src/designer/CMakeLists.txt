cmake_minimum_required(VERSION 2.8)

find_package(Qt4)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -frounding-math")
set(CMAKE_AUTOMOC TRUE)
include_directories(../../../3rdparty/)
include_directories(${QT_INCLUDES})
include_directories( ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

find_library(QGLVIEWER QGLViewer REQUIRED)
find_package(Qt4 COMPONENTS QtGui QtCore QtOpenGL REQUIRED)
set(QT_USE_QTOPENGL 1)
include(${QT_USE_FILE})
find_package(OpenGL REQUIRED)
include_directories(${OpenGL_INCLUDE_DIRS})


QT4_WRAP_UI(UI_HEADERS designer.ui scenesampler.ui)

FILE(GLOB SRCS *.cpp *.h)

add_executable(designer ${SRCS} ${UI_HEADERS})
target_link_libraries(designer ${QT_LIBRARIES} ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${QGLVIEWER} qjson -lCGAL -lgmp -lboost_thread GLU)
