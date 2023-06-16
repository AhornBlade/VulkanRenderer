cmake_minimum_required(VERSION 3.10)

project(VulkanRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

#third_party
add_subdirectory(third_party)

#source
add_subdirectory(source)

#examples
add_subdirectory(examples)