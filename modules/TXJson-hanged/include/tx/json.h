// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXJson

#pragma once
#include "impl/json/parser.hpp"
#include "impl/json/type_traits.hpp"
#include "impl/json/writer.hpp"

// things to add:
// operator<< for JosnValue / .str() function
// comments
// escaped character decodeing
// better error messages
// performance - reduce copy; linear (non-recursing) scan