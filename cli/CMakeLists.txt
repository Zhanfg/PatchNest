cmake_minimum_required(VERSION 3.10)
project("kpatch" LANGUAGES C)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${CMAKE_CURRENT_BINARY_DIR})

if(ANDROID)
    set(OPT_FLAGS "-ffunction-sections -fdata-sections -fvisibility=hidden -fvisibility-inlines-hidden -mllvm -polly -O3 -flto")
else()
    set(OPT_FLAGS "-O3")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11 ${OPT_FLAGS}")

if(ANDROID)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DANDROID")
endif()

set(SRCS
    src/kpatch.c
    src/kpm.c
    src/kpextension.c
    src/rehook.c
)

add_library(kp STATIC ${SRCS})
add_executable(kpatch ${SRCS} src/main.c)

# Transitional compatibility binary for PatchNest-Module's pinned
# KernelPatch-Public 0.13.x kernel side. It is intentionally a distinct build
# artifact: command IDs 0x1100/0x1101 are not semantically compatible with
# Next2026, so runtime mutation probing would be unsafe.
add_executable(kpatch-public1158 ${SRCS} src/main.c)
target_compile_definitions(kpatch-public1158 PRIVATE
    PATCHNEST_ABI_PUBLIC1158=1
    SUPERCALL_KEY_MAX_LEN=0x40
)

foreach(target kp kpatch kpatch-public1158)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Werror=format
        -Werror=implicit-function-declaration
        -Werror=incompatible-pointer-types
        -Werror=return-type
    )
endforeach()

foreach(target kp kpatch kpatch-public1158)
    target_link_options(${target} PRIVATE
        "-Wl,--build-id=none"
        "-Wl,-icf=safe,--lto-O3"
        "-Wl,-s,-x,--gc-sections"
        "-Wl,--no-undefined"
    )
endforeach()
