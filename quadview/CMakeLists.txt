cmake_minimum_required(VERSION 2.6)

project(quadview)

# There are lots of scripts with cmake for finding external libraries. 
# see /usr/local/share/cmake-2.6/Modules/Find*.cmake for more examples
find_package(OpenGL REQUIRED)
find_package(GLUT REQUIRED)
find_package(JPEG REQUIRED)

find_package(TIFF)
if(TIFF_FOUND)
	message("TIFF found")
	add_definitions(-DHAVE_TIFF)
else(TIFF_FOUND)
	message("TIFF not found, trying to build without TIFF format support")
	remove_definitions(-DHAVE_TIFF)
endif(TIFF_FOUND)

set(CMAKE_CXX_FLAGS "-g -Wall")

add_executable(quadview image.cpp quadview.cpp)

target_link_libraries(quadview
	${OPENGL_LIBRARY}
	${GLUT_LIBRARY}
	${JPEG_LIBRARY}
	${TIFF_LIBRARY})

