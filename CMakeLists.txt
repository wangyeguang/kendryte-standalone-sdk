# DO NOT MODIFY THIS FILE, IT WILL BE OVERRIDE!!!

# set this will supress some warnings
set(BUILDING_SDK "yes" CACHE INTERNAL "")

# basic config
# set(PROJ app)
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
header_directories(src/${PROJ})
header_directories(src/${PROJ}/include)
header_directories(src/${PROJ}/ai)
# header_directories(src/${PROJ}/utils/base64/include)
header_directories(src/${PROJ}/utils/cJSON/include)
# header_directories(src/${PROJ}/utils/jpeg_decode/include)
# header_directories(src/${PROJ}/utils/jpeg_encode/include)
# header_directories(src/${PROJ})
# header_directories(kendryte-standalone-demo/${PROJ})

# add_subdirectory(third_party/gsl-lite)
# add_subdirectory(third_party/mpark-variant)
# add_subdirectory(third_party/nlohmann_json)
# build library first
add_subdirectory(lib)

# 添加编译选项
# add_definitions(-DCONFIG_LOG_ENABLE=1)
# add_definitions(-DCONFIG_LOG_LEVEL=LOG_DEBUG)
# compile project
# add_source_files(src/${PROJ}/*.c src/${PROJ}/*.s src/${PROJ}/*.S src/${PROJ}/*.cpp src/${PROJ}/ai/*.c src/${PROJ}/utils/base64/*.c src/${PROJ}/utils/cJSON/*.c src/${PROJ}/utils/jpeg_decode/*.c src/${PROJ}/utils/jpeg_encode/*.c)
add_source_files(src/${PROJ}/*.c src/${PROJ}/*.s src/${PROJ}/*.S src/${PROJ}/*.cpp src/${PROJ}/ai/*.c)
# 
include(./cmake/executable.cmake)
# file(TO_CMAKE_PATH "${PROJ}/ai/libai.a" LIBAI_PATH)
# target_link_libraries(${PROJ} STATIC ${LIBAI_PATH})
# set(LIBRARY_PATH ${PORJ}/ai)
set(STATIC_LIB libai.a)
target_link_directories(${PROJ} PRIVATE  src/${PROJ}/ai)
target_link_libraries(${PROJ} ${STATIC_LIB})


# 添加链接选项
target_link_options(${PROJ} PRIVATE -Wl,--print-memory-usage)

add_compile_options(-Wl,--print-memory-usage)
# add_custom_command(TARGET app POST_BUILD
#         COMMAND ${CMAKE_OBJCOPY} -I binary -O elf64-littleriscv -B riscv $<TARGET_FILE:app>.bin $<TARGET_FILE:app>.elf)
# add_custom_target(print_size
#         ALL
#         DEPENDS app
#         COMMAND ${CMAKE_SIZE} ${PROJECT_BINARY_DIR}/${PROJ}.elf)   
