set(CMAKE_BUILD_TYPE Debug)

add_library(StaticIslands 
MODULE 
StaticIslands.cpp
AddHandshakeInterface.cpp
LoopUtils.cpp
../Synthesis/AutopilotParser.cpp)

SET(CMAKE_CXX_FLAGS "-fopenmp -fno-rtti -fPIC")
