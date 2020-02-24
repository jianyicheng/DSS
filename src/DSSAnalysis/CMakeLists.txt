set(CMAKE_BUILD_TYPE Debug)

set(libsrc
    callArgPrint.cpp
)

add_library(DSSAnalysis MODULE ${libsrc})

target_compile_options(DSSAnalysis PUBLIC -g3 -O0 -std=c++11 -pedantic -Wall -fPIC)
set(CMAKE_CXX_FLAGS "-fno-rtti -fPIC")
