set(CMAKE_BUILD_TYPE Debug)
add_library(loopFinder MODULE loopFinder.cpp)

target_compile_features(loopFinder PRIVATE cxx_range_for cxx_auto_type)
SET(CMAKE_CXX_FLAGS "-fopenmp -fno-rtti -fPIC")
