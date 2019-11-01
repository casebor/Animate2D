cmake_minimum_required(VERSION 2.8)
project(CGLab)

#--- Load the common configuration
include(common/config.cmake)
include(common/Eigen.cmake)
include(common/OpenGP.cmake)

#--- OpenGL configuration
include(common/OpenGL.cmake)
include(common/GLEW.cmake)
include(common/GLFW.cmake)

#--- Subprojects
add_subdirectory(P1)
#add_subdirectory(P2)
