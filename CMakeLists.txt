cmake_minimum_required(VERSION 3.16)
project(Chip8 LANGUAGES CXX C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(FetchContent)
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.5
    GIT_SHALLOW TRUE
    GIT_PROGRESS TRUE
)
FetchContent_MakeAvailable(SDL2)

add_executable(Chip8 chip8.cpp)
target_link_libraries(Chip8 PRIVATE SDL2::SDL2main)
target_link_libraries(Chip8 PRIVATE SDL2::SDL2)
target_compile_features(Chip8 PRIVATE cxx_std_20)