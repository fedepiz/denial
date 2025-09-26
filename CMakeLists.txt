cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(Main)

add_executable(Main src/main.cpp src/core.cpp src/ui.cpp)

target_include_directories(Main PRIVATE include)
target_include_directories(Main PRIVATE deps/include)

if (APPLE)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework CoreVideo -framework Cocoa -framework IOKit")
  #set(CMAKE_CXX_FLAGS "-framework CoreVideo -framework Cocoa -framework IOKit")
  target_link_directories(Main PRIVATE deps/libs/arm_64)
  target_link_libraries(Main PRIVATE glfw3 raylib)
endif()
