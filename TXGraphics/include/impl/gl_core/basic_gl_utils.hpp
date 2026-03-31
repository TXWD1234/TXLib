// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXGraphics
// File: basic_gl_utils.hpp

#pragma once

#include "tx/upset.hpp"
#include "tx/math.h"
#include "tx/type_traits.hpp"

namespace tx {
namespace RenderEngine {
using gid = u32;

struct mat2 {
	vec2 i, j;
};

template <class T>
struct glAttributeParameter {
	// use false_v to input the T, to make compiler don't evaluate this when parsing it, and only trigger it when instanciating it
	static_assert(false_v<T>, "tx::RE::glAttributeParameter_v: Unsupported type.");
};

template <>
struct glAttributeParameter<float> {
	using underlying = float;
	static constexpr u32 type = gl::enums::FLOAT;
	static constexpr u32 count = 1; // count of component / count of element
	static constexpr bool is_int = false;
};
template <>
struct glAttributeParameter<int> {
	using underlying = int;
	static constexpr u32 type = gl::enums::INT;
	static constexpr u32 count = 1; // count of component / count of element
	static constexpr bool is_int = true;
};
template <>
struct glAttributeParameter<u8> {
	using underlying = u8;
	static constexpr u32 type = gl::enums::UNSIGNED_BYTE;
	static constexpr u32 count = 1; // count of component / count of element
	static constexpr bool is_int = true;
};
template <>
struct glAttributeParameter<u32> {
	using underlying = u32;
	static constexpr u32 type = gl::enums::UNSIGNED_INT;
	static constexpr u32 count = 1; // count of component / count of element
	static constexpr bool is_int = true;
};
template <>
struct glAttributeParameter<u64> {
	using underlying = u64;
	static constexpr u32 type = gl::enums::UNSIGNED_INT64_ARB;
	static constexpr u32 count = 1; // count of component / count of element
	static constexpr bool is_int = true;
};
template <>
struct glAttributeParameter<tx::vec2> {
	using underlying = float;
	static constexpr u32 type = gl::enums::FLOAT;
	static constexpr u32 count = 2; // count of component / count of element
	static constexpr bool is_int = false;
};
template <>
struct glAttributeParameter<tx::Coord> {
	using underlying = int;
	static constexpr u32 type = gl::enums::INT;
	static constexpr u32 count = 2; // count of component / count of element
	static constexpr bool is_int = true;
};
template <>
struct glAttributeParameter<mat2> {
	using underlying = int;
	static constexpr u32 type = gl::enums::FLOAT;
	static constexpr u32 count = 4; // count of component / count of element
	static constexpr bool is_int = false;
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