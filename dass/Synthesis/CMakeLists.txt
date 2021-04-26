set(CMAKE_BUILD_TYPE Debug)

add_library(SynthesisPass MODULE Synthesis.cpp)
add_library(DirectSynthesisPass MODULE DirectSynthesis.cpp)
add_library(GetTimeOffsetPass MODULE DirectSynthesis.cpp AutopilotParser.cpp)
add_library(CollectVHDLPass MODULE Synthesis.cpp)
add_library(AddHandshakeInterfacePass MODULE AddHandshakeInterface.cpp ../StaticIslands/StaticIslands.cpp)

target_compile_features(AddHandshakeInterfacePass PUBLIC cxx_range_for cxx_auto_type)
target_compile_features(SynthesisPass PUBLIC cxx_range_for cxx_auto_type)
target_compile_features(CollectVHDLPass PUBLIC cxx_range_for cxx_auto_type)
SET(CMAKE_CXX_FLAGS "-fopenmp -fno-rtti -fPIC -lstdc++fs")
