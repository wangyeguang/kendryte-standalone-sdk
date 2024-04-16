# DO NOT MODIFY THIS FILE, IT WILL BE OVERRIDE!!!

# set this will supress some warnings
set(BUILDING_SDK "yes" CACHE INTERNAL "")
# basic config
if (NOT PROJ)
    get_filename_component(PROJ ${CMAKE_CURRENT_BINARY_DIR} DIRECTORY)
    get_filename_component(PROJ ${PROJ} NAME)
    string(REPLACE " " "_" PROJ ${PROJ})
    message(STATUS "PROJ not set, use ${PROJ} as PROJ. Also, you can set it manually. e.g. -DPROJ=hello_world")
else()
    message("PROJ = ${PROJ}")
endif ()
cmake_minimum_required(VERSION 3.0)
include(./cmake/common.cmake)
project(${PROJ} C CXX ASM)

# config self use headers
include(./cmake/macros.internal.cmake)
header_directories(${SDK_ROOT}/lib)
header_directories(${PROJ})
header_directories(${PROJ}/include)
# header_directories(src/${PROJ})
# header_directories(kendryte-standalone-demo/${PROJ})

add_subdirectory(third_party/gsl-lite)
add_subdirectory(third_party/mpark-variant)
add_subdirectory(third_party/nlohmann_json)
# build library first
add_subdirectory(lib)

# 添加编译选项
# add_compile_options(-Wl,--print-memory-usage)
# add_definitions(-DCONFIG_LOG_ENABLE=1)
# add_definitions(-DCONFIG_LOG_LEVEL=LOG_DEBUG)
# compile project
# add_source_files(src/${PROJ}/*.c src/${PROJ}/*.s src/${PROJ}/*.S src/${PROJ}/*.cpp)
add_source_files(${PROJ}/src/*.c ${PROJ}/src/*.cpp)
# add_source_files(kendryte-standalone-demo/${PROJ}/*.c kendryte-standalone-demo/${PROJ}/*.s kendryte-standalone-demo/${PROJ}/*.S kendryte-standalone-demo/${PROJ}/*.cpp)
include(./cmake/executable.cmake)

# add_custom_command(TARGET app POST_BUILD
#         COMMAND ${CMAKE_OBJCOPY} -I binary -O elf64-littleriscv -B riscv $<TARGET_FILE:app>.bin $<TARGET_FILE:app>.elf)
# add_custom_target(print_size
#         ALL
#         DEPENDS app
#         COMMAND ${CMAKE_SIZE} ${PROJECT_BINARY_DIR}/${PROJ}.elf)   