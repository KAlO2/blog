cmake_minimum_required(VERSION 2.6)

project(quadview)

# There are lots of scripts with cmake for finding external libraries. 
# see /usr/local/share/cmake-2.6/Modules/Find*.cmake for more examples
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(JPEG REQUIRED)

set(LIBRARIES ${OPENGL_gl_LIBRARY} ${OPENGL_glu_LIBRARY} ${GLUT_LIBRARY} ${JPEG_LIBRARY})
set(DIRECTORIES ${GLUT_INCLUDE_DIR} ${JPEG_INCLUDE_DIR})

find_package(TIFF)
if(TIFF_FOUND)
	message("TIFF found")
	add_definitions(-DHAVE_TIFF)
	set(LIBRARIES ${LIBRARIES} ${TIFF_LIBRARY})
	set(DIRECTORIES ${DIRECTORIES} ${TIFF_INCLUDE_DIR})
else(TIFF_FOUND)
	message("TIFF not found, trying to build without TIFF format support")
	remove_definitions(-DHAVE_TIFF)
endif(TIFF_FOUND)

if(WIN32)
	link_directories("C:/Program Files (x86)/Microsoft Visual Studio 12.0/VC/lib")
else()
	set(CMAKE_CXX_FLAGS "-g -Wall")
endif()

include_directories(${DIRECTORIES})
add_executable(quadview image.cpp quadview.cpp)

target_link_libraries(quadview ${LIBRARIES})