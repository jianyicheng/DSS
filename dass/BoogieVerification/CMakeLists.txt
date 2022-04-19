set(CMAKE_BUILD_TYPE Debug)

add_library(BoogieVerification 
MODULE 
BoogieCodeGen.cpp
LoopInterchangeCheck.cpp)

SET(CMAKE_CXX_FLAGS "-fopenmp -fno-rtti -fPIC")
