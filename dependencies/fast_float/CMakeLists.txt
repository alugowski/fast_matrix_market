cmake_minimum_required(VERSION 3.9)
project(fast_float VERSION 3.9.0 LANGUAGES CXX)

add_library(fast_float INTERFACE)
add_library(FastFloat::fast_float ALIAS fast_float)
target_include_directories(
  fast_float
  INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(fast_float INTERFACE cxx_std_11)
if(FASTFLOAT_SANITIZE)
  target_compile_options(fast_float INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  target_link_libraries(fast_float INTERFACE -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fno-sanitize-recover=all)
  if (CMAKE_COMPILER_IS_GNUCC)
    target_link_libraries(fast_float INTERFACE -fuse-ld=gold)
  endif()
endif()
if(MSVC_VERSION GREATER 1910)
  target_compile_options(fast_float INTERFACE /permissive-)
endif()

