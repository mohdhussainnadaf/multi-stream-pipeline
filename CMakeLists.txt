cmake_minimum_required(VERSION 3.14)
project(MultiStreamPipeline CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

execute_process(COMMAND brew --prefix OUTPUT_VARIABLE BREW_PREFIX OUTPUT_STRIP_TRAILING_WHITESPACE)

find_package(OpenCV REQUIRED)
find_package(Threads REQUIRED)

find_library(ONNXRUNTIME_LIB onnxruntime HINTS ${BREW_PREFIX}/lib /opt/homebrew/lib)
find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h HINTS ${BREW_PREFIX}/include/onnxruntime ${BREW_PREFIX}/include /opt/homebrew/include/onnxruntime)

include_directories(
    ${OpenCV_INCLUDE_DIRS}
    ${ONNXRUNTIME_INCLUDE_DIR}
)

add_executable(pipeline_engine pipeline.cpp)

target_link_libraries(pipeline_engine
    PRIVATE
    ${OpenCV_LIBS}
    ${ONNXRUNTIME_LIB}
    Threads::Threads
)
