function(generate_shaders SHADER_DIR TARGET_NAME)
	set(SPV_DIR ${CMAKE_CURRENT_BINARY_DIR}/spv)
	set(HEADER_DIR ${CMAKE_CURRENT_BINARY_DIR}/header)
	find_package(Vulkan REQUIRED)
	set(GLSL_COMPILER_BIN ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE})
	file(GLOB SHADERS ${SHADER_DIR}/*.vert ${SHADER_DIR}/*.frag ${SHADER_DIR}/*.comp ${SHADER_DIR}/*.geom ${SHADER_DIR}/*.tesc ${SHADER_DIR}/*.tese ${SHADER_DIR}/*.mesh ${SHADER_DIR}/*.task ${SHADER_DIR}/*.rgen ${SHADER_DIR}/*.rchit ${SHADER_DIR}/*.rmiss)
	
	foreach(SHADER IN LISTS SHADERS)
		get_filename_component(SHADER_NAME ${SHADER} NAME)
		string(REPLACE "." "_" HEADER_NAME ${SHADER_NAME})
		string(TOUPPER ${HEADER_NAME} GLOBAL_SHADER_VAR)
		set(SPV_FILE "${SPV_DIR}/${SHADER_NAME}.spv")
		set(SPV_TARGET "${HEADER_NAME}_spv")
		set(HEADER_FILE "${HEADER_DIR}/${HEADER_NAME}.hpp")
		set(HEADER_TARGET "${HEADER_NAME}_header")
		set(GLSL_COMPILE_COMMAND ${GLSL_COMPILER_BIN} -I${SHADER_DIR} -V100 -o ${SPV_FILE} ${SHADER})

		add_custom_command(OUTPUT ${SPV_FILE}
			COMMAND ${GLSL_COMPILE_COMMAND}
			DEPENDS ${SHADER}
			COMMENT "Compiling ${FILENAME}: ${GLSL_COMPILE_COMMAND}")
		add_custom_target(${SPV_TARGET} DEPENDS ${SPV_FILE})

		add_custom_command(OUTPUT ${HEADER_FILE}
			COMMAND ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/generate_shader ${SPV_FILE} ${HEADER_FILE}
			DEPENDS ${SPV_FILE} generate_shader
			COMMENT "Generating ${HEADER_NAME}")
		add_custom_target(${HEADER_TARGET} DEPENDS ${HEADER_FILE})
		add_dependencies(${HEADER_TARGET} ${SPV_TARGET})
		add_dependencies(${HEADER_TARGET} generate_shader)

		add_dependencies(${TARGET_NAME} ${HEADER_TARGET})
	endforeach()

	target_include_directories(${TARGET_NAME}
		PUBLIC ${HEADER_DIR})

endfunction()