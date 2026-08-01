// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/parted_arr.hpp"
#include "impl/circular_arr.hpp"
#include "impl/sorted_arr.hpp"
#include "impl/data_utils.hpp"
#include "impl/value_group.hpp"
#include "impl/circular_queue.hpp"
#include "impl/binary_set_view.hpp"
#include "impl/avl_tree.hpp"
#include "impl/grow_array.hpp"
#include "impl/basic_storage.hpp"
#include "impl/ring_buffer.hpp"

/**
 * Terminology:
 * 
 * - Overlay:
 *   A data structure that don't own the data, but manipulates the data.
 *   It contains the logic to operate the data.
 * 
 * - View:
 *   A data structure that don't own the data, and don't manipulate the data
 *   either.
 *   Read only, anyone use the data structure will not be able to manipulate
 *   the data
 * 
 * - Span:
 *   A data structure that don't own the data, and don't manipulate the data
 *   by itself.
 *   But the user of the data structure can manipulate the data by calling
 *   methods of the data structure or geting the reference to the raw data.
 *   
 * 
 */

/**
 * The Overlay Pattern
 * Since overlay classes don't own the data, a memory managing wrapper have to
 * be implemented for every overlay data structure.
 * The implementation of the wrapper class are all boilerplate, therefore it
 * could be directly copied from another implementation and just rename the
 * class name.
 * 
 * The pattern is designed to make the implementation of the memory managing
 * wrapper identical across every wrapper.
 * The wrapper class will inherit from the overlay class, thereby can be freely
 * converted in to overlay for operations.
 * The core logic that is required for the lifetime management is implmented in
 * the overlay class, exposed in `protected` scope.
 * 
 * The Stardard Specialization:
 * - The Overlay Class:
 *   - need to expose these functions in `protected` scope:
 *     - null_impl()
 *         invalidate the class (state and data pointer)
 *     - isNull_impl()
 *         check if class is invalid
 *     - swap_impl(Overlay<T>&)
 *         swap all the state data with another overlay object - used to
 *         support the Copy-and-Swap mechanism
 *     - copy_impl(const Overlay<T>&)
 *         copy all the meta data from another overlay object
 *     - destruct_impl()
 *         destroy and clean up the containing data. called when the wrapper
 *         is destructing
 * - The Memory Managing Wrapper
 *   - copy the boilerplate code from any other implementation and rename.
 */