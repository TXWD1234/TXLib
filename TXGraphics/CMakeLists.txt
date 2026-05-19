# Copyright (c) 2025 TXLib. Licensed under the MIT License.
# Module: TXGraphics

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/dependency/glad")
find_package(glfw3 CONFIG REQUIRED)
find_package(OpenGL REQUIRED)

tx_txlib_module(
	LIB_TYPE STATIC
	PUBLIC_HEADERS
		graphics.h
	IMPL_HEADERS
		"gl_core/basic_gl_utils.hpp"
		"gl_core/buffer.hpp"
		"gl_core/draw_call_manager.hpp"
		"gl_core/fence_manager.hpp"
		"gl_core/fence.hpp"
		"gl_core/shader_manager.hpp"
		"gl_core/shader.hpp"
		"gl_core/texture.hpp"
		"gl_core/utility.hpp"
		"gl_core/vertex_attribute_manager.hpp"

		"render_engine/log.hpp"
		"render_engine/render_engine.hpp"
		"render_engine/renderer_framework.hpp"
		"render_engine/renderer.hpp"
		"render_engine/texture_manager.hpp"

		"utility/rgb.hpp"

		"framework/framework.hpp"
	SOURCES
		"gl_core/shader.cpp"
		"gl_core/texture.cpp"

		"render_engine/render_engine.cpp"
		"render_engine/renderer_framework.cpp"
		"render_engine/renderer.cpp"

		"framework/framework.cpp"	
)

target_link_libraries(TXGraphics PUBLIC 
	glfw
	glad
	OpenGL::GL
)