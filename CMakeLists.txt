cmake_minimum_required(VERSION 3.0)
project(rtiaw)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wextra -Wall -pedantic")
set(CMAKE_CXX_EXTENSIONS OFF)
# set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)

add_executable(ray_tracer main.cpp)
target_link_libraries(ray_tracer sfml-graphics sfml-window sfml-system)
