cmake_minimum_required(VERSION 4.2)

set(libs "${CMAKE_SOURCE_DIR}/libs")
set(src "${CMAKE_SOURCE_DIR}/src")

project(TXProject)

# install
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Install prefix to ~/.local" FORCE)
endif()

# TXLib
include("../add_txlib.cmake")

tx_add_txlib(
	LOCAL_DIR
		"../"
	BIN_DIR
		"./build/TXLib"
	MODULES
		TXMath
		TXMap
		TXJson
		TXGrid
		TXResource
		TXData
		TXGraphics
		TXUtility
)

add_executable(TXProject 
	"${src}/main.cpp"
	"${src}/Project.hpp"
	"${src}/utility_dump.hpp")
target_include_directories(TXProject PRIVATE 
	"${CMAKE_SOURCE_DIR}"
	"${libs}"
	"${src}"
)

# project settings
target_compile_features(TXProject PUBLIC cxx_std_23)

# dependencies
target_link_libraries(TXProject PRIVATE 
	TXMath
	TXMap
	TXJson
	TXGrid
	TXResource
	TXGraphics
	TXData
	stb_image
)

# optimization
include("../TXCMake/set_compile_flags.cmake")
tx_set_compile_flags(TXProject PRIVATE)

# build
set_target_properties(TXProject PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_BINARY_DIR}/bin"
	RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/bin"
)

# executable environment
include("../TXCMake/link_dir_to_bin.cmake")
tx_link_dir_to_bin(TXProject "${CMAKE_SOURCE_DIR}/assets")

install(TARGETS TXProject
    RUNTIME DESTINATION bin
)