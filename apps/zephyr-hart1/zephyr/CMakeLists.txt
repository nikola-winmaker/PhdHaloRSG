cmake_minimum_required(VERSION 3.20.0)
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(hart1_zephyr_app)

set(APP_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(USE_HALO 0 CACHE STRING "Build Zephyr Hart1 with HALO-generated sources")

if(DEFINED ENV{COMMON_DIR} AND NOT "$ENV{COMMON_DIR}" STREQUAL "")
  set(COMMON_DIR $ENV{COMMON_DIR})
else()
  set(COMMON_DIR ${APP_ROOT}/../../src/common)
endif()

if(USE_HALO)
  set(DEPS_DIR ${APP_ROOT}/deps/halo/codegen/riscv64_h1_zephyr)
else()
  set(DEPS_DIR ${APP_ROOT}/deps/classical)
endif()

file(GLOB DEPS_SRCS ${DEPS_DIR}/src/*.c)
file(GLOB COMMON_SRCS ${COMMON_DIR}/*.c)

target_sources(app PRIVATE src/main.c ${DEPS_SRCS} ${COMMON_SRCS})
target_include_directories(app PRIVATE src ${DEPS_DIR}/include ${COMMON_DIR})
target_compile_definitions(app PRIVATE USE_HALO=${USE_HALO})
