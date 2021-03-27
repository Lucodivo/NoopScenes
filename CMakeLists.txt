cmake_minimum_required(VERSION 3.17)
project(NoopScenes)

set(CMAKE_CXX_STANDARD 14)

set(LIB_DIR "C:/developer/dependencies/libs")
set(INCL_DIR "C:/developer/dependencies/include")
set(EXT_DIR "${CMAKE_SOURCE_DIR}/external")

link_directories(${LIB_DIR})
include_directories(${INCL_DIR} ${EXT_DIR} "${EXT_DIR}/stb" "${EXT_DIR}/glm")

add_library(glad STATIC "${EXT_DIR}/glad/glad.c" )

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
    add_compile_options(/W3)
else ()
    add_compile_options(-Wall -Wall -Wpedantic)
endif()

add_executable(NoopScenes src/main.cpp)

# Put all libraries into a variable
set(LIBS glfw3-x64-d opengl32 glad)

# Define the link libraries
target_link_libraries(${PROJECT_NAME} ${LIBS})