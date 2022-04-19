set(CMAKE_BUILD_TYPE Debug)

add_library(Frontend 
MODULE 
FrontendDASS.cpp
FrontendLLVM.cpp)

SET(CMAKE_CXX_FLAGS "-fopenmp -fno-rtti -fPIC")
