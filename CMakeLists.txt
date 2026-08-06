cmake_minimum_required(VERSION 3.22)
project(PatchNestCLI VERSION 0.13.5 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(PATCHNEST_SOURCES
    src/kpatch.c
    src/kpm.c
    src/kpextension.c
    src/rehook.c
)

add_library(patchnest_cli STATIC ${PATCHNEST_SOURCES})
add_executable(kpatch-android ${PATCHNEST_SOURCES} src/main.c)

target_include_directories(patchnest_cli PUBLIC src)
target_include_directories(kpatch-android PRIVATE src)

target_compile_options(patchnest_cli PRIVATE -O3 -ffunction-sections -fdata-sections -fvisibility=hidden)
target_compile_options(kpatch-android PRIVATE -O3 -ffunction-sections -fdata-sections -fvisibility=hidden)

target_link_options(patchnest_cli PRIVATE -Wl,--build-id=none -Wl,-s,--gc-sections -Wl,--no-undefined)
target_link_options(kpatch-android PRIVATE -Wl,--build-id=none -Wl,-s,--gc-sections -Wl,--no-undefined)

set_target_properties(kpatch-android PROPERTIES OUTPUT_NAME kpatch-android)
