cmake_minimum_required(VERSION 3.24)

project(VulkanRenderer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

#if(CMAKE_SYSTEM_NAME MATCHES "Windows")
#	add_definitions(-DVK_USE_PLATFORM_WIN32_KHR)
#	add_definitions(-DGLFW_EXPOSE_NATIVE_WIN32)
#elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
#	add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
#	add_definitions(-DGLFW_EXPOSE_NATIVE_X11)
#endif()

if(CMAKE_BUILD_TYPE MATCHES "Debug")
	add_definitions(-DVULKAN_RENDERER_DEBUG)
endif()

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

#third_party
add_subdirectory(third_party)

#tools
add_subdirectory(tools)

#source
add_subdirectory(source)

#cmake script
add_subdirectory(script)

#test
add_subdirectory(test)