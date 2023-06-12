cmake_minimum_required(VERSION 3.4)
project(ryu VERSION 2.0 LANGUAGES C)
include(GNUInstallDirs)

# ryu library
add_library(ryu
        ryu/f2s.c
        ryu/f2s_full_table.h
        ryu/f2s_intrinsics.h
        ryu/d2s.c
        ryu/d2fixed.c
        ryu/d2fixed_full_table.h
        ryu/d2s_full_table.h
        ryu/d2s_small_table.h
        ryu/d2s_intrinsics.h
        ryu/digit_table.h
        ryu/common.h
        ryu/ryu.h)

# This directory is the include root because the headers are in ryu/ and are included as "ryu/*.h"
target_include_directories(ryu PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)

# add alias so the project can be used with add_subdirectory
add_library(ryu::ryu ALIAS ryu)

# Specify what to install if using CMake to install ryu.
install(TARGETS ryu LIBRARY)
install(FILES ryu/ryu.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ryu)
