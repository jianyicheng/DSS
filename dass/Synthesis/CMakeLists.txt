set(CMAKE_BUILD_TYPE Debug)

add_library(Synthesis 
MODULE
VHDLPortParser.cpp
AutopilotParser.cpp
DotBuffering.cpp
Synthesis.cpp)

SET(CMAKE_CXX_FLAGS "-fopenmp -fno-rtti -fPIC -lstdc++fs")
