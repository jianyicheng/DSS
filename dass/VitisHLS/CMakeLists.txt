set(CMAKE_BUILD_TYPE Debug)

add_library(VitisHLS 
MODULE 
AddTopAttributes.cpp)

SET(CMAKE_CXX_FLAGS "-fopenmp -fno-rtti -fPIC")
