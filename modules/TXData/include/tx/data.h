// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/parted_array.hpp"
#include "impl/circular_array.hpp"
#include "impl/sorted_array.hpp"
#include "impl/data_utils.hpp"
#include "impl/value_group.hpp"
#include "impl/circular_queue.hpp"
#include "impl/binary_set_view.hpp"
#include "impl/avl_tree.hpp"
#include "impl/grow_array.hpp"
#include "impl/basic_storage.hpp"
#include "impl/ring_buffer.hpp"
#include "impl/packed_parted_array.hpp"
#include "impl/hash_set.hpp"
#include "impl/freelist.hpp"

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
 *   methods of the data structure or getting the reference to the raw data.
 * 
 * - Buffer
 *   A contiguous chunck of raw memory, without any algorithm above it or
 *   operating it.
 * 
 * - Array:
 *   A buffer that has an algorithm above it or operating it, making it perform
 *   certain behavior. An Array have to be encapsulated in a class.
 * 
 * - Vector:
 *   An array with the ability to self reallocate / self resize. Also known as
 *   a DynamicArray.
 * 
 * 
 *   
 * 
 */

/**
 * # The Overlay Pattern
 * 
 * ## Memory Managing Wrapper
 * ### Specialization:
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
 * 
 * ### Explaination
 * Since overlay classes don't own the data, a memory managing wrapper have to
 * be implemented for every overlay data structure.
 * The implementation of the wrapper class are all boilerplate, therefore it
 * could be directly copied from another implementation and just rename the
 * class name.
 * 
 * The pattern is designed to make the implementation of the memory managing
 * wrapper identical across every wrapper.
 * The wrapper class inherits from the overlay class, allowing it to be freely
 * converted to an overlay for operations.
 * The core logic that is required for the lifetime management is implmented in
 * the overlay class, exposed in `protected` scope.
 * 
 * 
 * ## StateStorage
 * Every overlay class must have a nested public `StateStorage` struct defined,
 * and take a pointer of `StateStorage` as the last argument of it's
 * constructor.
 * Internally, a `State_impl` struct should be defined (though the name could
 * vary dependent on implementation, not encouraged though. It's also optional,
 * since a overlay class without state data is also possible), and every state
 * variables must be stored in that `State_impl` struct.
 * The definition of "state variable": Any member variable that may be mutated
 * after construction.
 * `StateStorage` is a opaque struct that is aligned and sized corresponding to
 * the internal `State_impl` struct.
 * During construction, a `State_impl` object will be constructed on the memory
 * of the provided `StateStorage` object pointer.
 * 
 * ### AliasObject
 * Terminology: "AliasObject" defines the "objects that share the same
 * buffer and state object". They are linked with each other, any operations
 * performed on one object are visible acorss every other alias objects of
 * that object.
 * 
 * ## Null State
 * A default constructed overlay class is in null state.
 * The null state condition varies on data structures, but a value of `nullptr`
 * for `m_data` (the data pointer of the memory the overlay class is managing),
 * a value of 0 for `m_size` (the size of the memory) and a value of `nullptr`
 * for `m_state` guarantee null state.
 * Every overlay class must provide a `valid()` public getter function to check
 * it's null state.
 * There must be an assert for every public method of the class, asserting the
 * validity of the object. The assert preset `tx::impl::assert::object_valid`
 * may, and is encouraged to be used.
 * During construction of the object, if invalid parameter was provided, the
 * overlay may result in Null State.
 * During the lifetime of the object, it is not allowed to enter Null State by
 * user called operations.
 * After destruction (`.destruct()` method called), the object is still not in 
 * null state, but instead in a "call-and-UB" state. The object should be
 * immediately destroyed after called `.destruct()` anyways.
 * The Null State is a representation of an invalid state. Therefore when
 * nothing had went wrong, an object should never enters the Null State.
 * 
 * ## Relocation
 * Every overlay class should provide a set of relocation method for each of
 * their buffer. A set of relocation method consist of 2 methods: `rebind` and
 * `relocate`, in which rebind does not move the data over while relocate does.
 * When there are multiple buffers, the name of the buffer is added as suffix
 * of the method name (eg. `rebindData` / `rebindMeta`). When there's only one
 * buffer, the suffix can be omited and the method name can simply be `rebind`/
 * `relocate`.
 * The purpose of relocation methods are to preserve the state while switching
 * buffer, since there's no way to construct a new overlay object with existing
 * state.
 */