add_library(spirv-reflect-wrapper spirv_reflect_wrapper.cpp SPIRV-Reflect/spirv_reflect.cpp)
target_include_directories(spirv-reflect-wrapper
    PUBLIC .)
