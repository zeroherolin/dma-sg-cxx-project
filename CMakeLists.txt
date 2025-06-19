cmake_minimum_required(VERSION 3.10)
project(dma-sg)

# Set compile options
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Include directories
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# Add reg_rw library (C language)
add_library(reg_rw STATIC
    ${PROJECT_SOURCE_DIR}/tools/reg_rw.c
)

# Add dma_controller library (C++)
add_library(dma_controller STATIC
    ${PROJECT_SOURCE_DIR}/dma-sg/dma_controller.cpp
)

# Link reg_rw to dma_controller
target_link_libraries(dma_controller
    PRIVATE reg_rw
)

# Add test program
add_executable(dma_test
    ${PROJECT_SOURCE_DIR}/test/test.cpp
)

# Link test program to library
target_link_libraries(dma_test
    PRIVATE dma_controller
)

# Set executable output directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)
