// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/parted_arr.hpp"
#include "impl/circular_arr.hpp"
#include "impl/sorted_arr.hpp"
#include "impl/data_utils.hpp"
#include "impl/value_group.hpp"
#include "impl/circular_queue.hpp"
#include "impl/avl_tree.hpp"
#include "impl/grow_array.hpp"
#include "impl/basic_storage.hpp"

/**
 * Terminology:
 * 
 * - Overlay:
 *   A data structure that don't own the data, but manipulates the data.
 *   Overlay contains the logic, all it's member should be `protected`. To
 *   create a full data structure from an overlay, just inherit the overlay
 *   class and overwrite the constructor and destructor
 * 
 * - View:
 *   A data structure that don't own the data, and don't manipulate the data
 *   either.
 *   Read only, anyone use the data structure will not be able to manipulate
 *   the data
 * 
 * - Span:
 *   A data structure that don't own the data, and don't manipulate the data by itself.
 *   But the user of the data structure can manipulate the data by calling methods of the data structure or geting the reference to the raw data.
 *   
 * 
 */