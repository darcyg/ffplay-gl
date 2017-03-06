cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(
        ../../ffmpeg/include
)

aux_source_directory(
        ../../ffplay SRC
)

link_directories(
        ../../ffmpeg/lib
)

link_libraries(
        libavcodec.a
        libavdevice.a
        libavfilter.a
        libavformat.a
        libavutil.a
        libswresample.a
        libswscale.a
)

set(SOURCE_FILES
        ${SRC})

add_executable(ffplay ${SOURCE_FILES})