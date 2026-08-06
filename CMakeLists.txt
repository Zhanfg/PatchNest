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

foreach(target kp kpatch)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
        -Werror=format
        -Werror=implicit-function-declaration
        -Werror=incompatible-pointer-types
        -Werror=return-type
    )
endforeach()

target_link_options(kp PRIVATE
    "-Wl,--build-id=none"
    "-Wl,-icf=safe,--lto-O3"
    "-Wl,-s,-x,--gc-sections"
    "-Wl,--no-undefined"
)
target_link_options(kpatch PRIVATE
    "-Wl,--build-id=none"
    "-Wl,-icf=safe,--lto-O3"
    "-Wl,-s,-x,--gc-sections"
    "-Wl,--no-undefined"
)
