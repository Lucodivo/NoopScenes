cmake_minimum_required(VERSION 3.17)
project(NoopScenes)

set(CMAKE_CXX_STANDARD 14)

if (NOT WIN32)
    message([FATAL_ERROR] "This project currently requires Windows and has not been ported to any other platform.")
endif()

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/W3)
else ()
    #add_compile_options(-Wall -Wall -Wpedantic)
endif()

# These next three lines are what you need to edit to build this project
set(GLFW_HEADER_LOCATIONS "C:/developer/dependencies/include/GLFW") # Directory that contains
set(GLFW_LIB_LOCATION "C:/developer/dependencies/libs") # Directory that contains GLFW3 static library
set(GLFW_LIB glfw3-x64-d.lib) # Actual name of compiled glfw3 static library (default set for "glfw3-x64-d.lib")

set(LIB_DIR ${GLFW_LIB_LOCATION})
set(INCL_DIR ${GLFW_HEADER_LOCATIONS})
set(EXT_DIR "${CMAKE_SOURCE_DIR}/external")

link_directories(${LIB_DIR})
include_directories(${INCL_DIR} ${EXT_DIR} "${EXT_DIR}/stb" "${EXT_DIR}/glad")

add_library(glad STATIC "${EXT_DIR}/glad/glad.c" )

add_executable(NoopScenes src/main.cpp)

add_executable(TestingGround src/testing_ground/testing_ground.cpp)

# Put all libraries into a variable
set(LIBS opengl32 glad ${GLFW_LIB})

# Define the link libraries
target_link_libraries(${PROJECT_NAME} ${LIBS})