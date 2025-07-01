cmake_minimum_required(VERSION 3.21)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
project(gb VERSION 0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE SRC_FILES src/*.cpp)

find_package(SDL3 CONFIG REQUIRED)

add_executable(gb ${SRC_FILES})
target_link_libraries(gb
	SDL3::SDL3
)