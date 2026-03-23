// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: basic_gl_utils.hpp

#pragma once

#include "tx/upset.hpp"
#include "tx/math.h"
#include "impl/type_traits.hpp"

namespace tx {
namespace RenderEngine {
using gid = u32;

template <class T>
struct glAttributeParameter {
	// use false_v to input the T, to make compiler don't evaluate this when parsing it, and only trigger it when instanciating it
	static_assert(false_v<T>, "tx::RE::glAttributeParameter_v: Unsupported type.");
};

template <>
struct glAttributeParameter<float> {
	static constexpr u32 type = 0x1406; // GL_FLOAT
	static constexpr u32 count = 1; // count of component / count of element
	static constexpr bool is_int = false;
};
template <>
struct glAttributeParameter<int> {
	static constexpr u32 type = 0x1404; // GL_INT
	static constexpr u32 count = 1; // count of component / count of element
	static constexpr bool is_int = true;
};
template <>
struct glAttributeParameter<u8> {
	static constexpr u32 type = 0x1401; // GL_UNSIGNED_BYTE
	static constexpr u32 count = 1; // count of component / count of element
	static constexpr bool is_int = true;
};
template <>
struct glAttributeParameter<u32> {
	static constexpr u32 type = 0x1405; // GL_UNSIGNED_INT
	static constexpr u32 count = 1; // count of component / count of element
	static constexpr bool is_int = true;
};
template <>
struct glAttributeParameter<tx::vec2> {
	static constexpr u32 type = 0x1406; // GL_FLOAT
	static constexpr u32 count = 2; // count of component / count of element
	static constexpr bool is_int = false;
};
template <>
struct glAttributeParameter<tx::Coord> {
	static constexpr u32 type = 0x1404; // GL_INT
	static constexpr u32 count = 2; // count of component / count of element
	static constexpr bool is_int = true;
};

template <class T>
using glAttribParam = glAttributeParameter<T>;

template <class T>
inline constexpr u32 glType = glAttributeParameter<T>::type;
template <class T>
inline constexpr u32 glComponentCount = glAttributeParameter<T>::count;
template <class T>
inline constexpr bool glTypeIsInt = glAttributeParameter<T>::is_int;





} // namespace RenderEngine
}; // namespace tx