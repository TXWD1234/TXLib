// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXData

#pragma once
#include "impl/data_foundation.hpp"
#include "impl/allocator.hpp"
#include "impl/basic_storage.hpp"
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <concepts>
#include <utility>
#include <span>


// ================ Overlay Pattern Wrapper Utilities ================
namespace tx::impl {
// concept for overlay class
template <class O>
concept overlay = requires(O o) {
	{ o.valid() } -> std::same_as<bool>;
	o.destruct();
	typename O::StateStorage;
};

template <template <class...> class Overlay, class... TArgs>
    requires impl::overlay<Overlay<TArgs...>>
class OverlayInlined
    : public Overlay<TArgs...>,
      private impl::StorageHolder<
          typename Overlay<TArgs...>::StateStorage> {
private:
	using Base = Overlay<TArgs...>;
	using Storage = impl::StorageHolder<
	    typename Overlay<TArgs...>::StateStorage>;

public:
	template <class... Args>
	    requires std::constructible_from<
	                 Base, Args..., typename Base::StateStorage*>
	OverlayInlined(Args&&... args)
	    : Storage{},
	      Base(std::forward<Args>(args)..., &this->storage) {}
	~OverlayInlined() { this->destruct(); }

	OverlayInlined(const OverlayInlined&) = delete;
	OverlayInlined& operator=(const OverlayInlined&) = delete;
	OverlayInlined(OverlayInlined&& other) = delete;
	OverlayInlined& operator=(OverlayInlined&& other) = delete;
};

template <tx::allocator Allocator, class... Args>
class OverlayMMWBuffers {
public:
	static constexpr size_t DefaultBufferSize = 64;
	static constexpr size_t BufferCount = sizeof...(Args);

	using Storage = std::tuple<tx::BasicStorage<Args, Allocator>...>;
	Storage buffers;
	Allocator allocator;

	template <std::same_as<u32>... SizeT>
	OverlayMMWBuffers(Allocator alloc, SizeT... sizes)
	    : buffers(std::make_tuple(
	          tx::BasicStorage<Args, Allocator>{ sizes, alloc }...)),
	      allocator(alloc) {}
	OverlayMMWBuffers(Allocator alloc)
	    : buffers(std::make_tuple(
	          tx::BasicStorage<Args, Allocator>{ DefaultBufferSize, alloc }...)),
	      allocator(alloc) {}

	OverlayMMWBuffers(const OverlayMMWBuffers& other)
	    : buffers(std::apply(
	          [&](const tx::BasicStorage<Args, Allocator>&... memStorage) {
		          return std::make_tuple(tx::BasicStorage<Args, Allocator>{
		              memStorage.size, other.allocator }...);
	          },
	          other.buffers)),
	      allocator(other.allocator) {}
	OverlayMMWBuffers(OverlayMMWBuffers&& other)
	    : buffers(std::move(other.buffers)),
	      allocator(std::move(other.allocator)) {}

	OverlayMMWBuffers& operator=(OverlayMMWBuffers other) {
		std::swap(buffers, other.buffers);
		std::swap(allocator, other.allocator);
		return *this;
	}
};
// Memory Managing Wrapper
/**
 * Template Parameters:
 * - `Overlay`: The overlay class template
 * - `BufferTypes`: The type of the buffers the overlay class requires
 * - `TArgs`: The extra template parameters the overlay class requires
 * - `Allocator`: The allocator used to manage memory. The type does not matter.
 *                The default parameter is std::allocator
 */
template <template <class...> class Overlay,
          tx::type_list BufferTypes,
          class... TArgs>
    requires impl::overlay<Overlay<TArgs...>>
class OverlayMMW
    : public OverlayInlined<Overlay, TArgs...>,
      private tx::type_list_append_front_t<
          BufferTypes,
          std::conditional_t<
              tx::allocator<tx::type_list_back_t<tx::type_list_t<TArgs...>>>,
              tx::type_list_back_t<tx::type_list_t<TArgs...>>, std::allocator<u32>>>::
          template apply_t<OverlayMMWBuffers> {
private:
	// static constexpr bool AllocatorProvided =
	//     tx::allocator<tx::type_list_back_t<tx::type_list_t<TArgs...>>>;

	using Allocator = std::conditional_t<
	    tx::allocator<tx::type_list_back_t<tx::type_list_t<TArgs...>>>,
	    tx::type_list_back_t<tx::type_list_t<TArgs...>>, std::allocator<u32>>;
	using Base = OverlayInlined<Overlay, TArgs...>;
	using BufferStorage = typename tx::type_list_append_front_t<
	    BufferTypes, Allocator>::
	    template apply_t<OverlayMMWBuffers>;

	static consteval bool check_interface() {
		return requires(OverlayMMW& self, Base& base_ref, const Base& base_cref) {
			self.null_impl();
			{ self.isNull_impl() } -> std::convertible_to<bool>;
			self.swap_impl(base_ref);
			self.copy_impl(base_cref);
			self.destruct_impl();
		};
	}
	static_assert(
	    check_interface(),
	    "TXData Overlay Pattern: An Overlay class must provide accessible"
	    "protected functions according to the standard.");

private:
	template <size_t... Is>
	static Base makeBase_impl(
	    typename BufferStorage::Storage buffers,
	    std::index_sequence<Is...>) {
		return Base(std::span(
		    std::get<Is>(buffers).data,
		    std::get<Is>(buffers).size)...);
	}

public:
	/**
	 * Parameters:
	 * - `sizes`: First n arguments are the buffer size, respectively for each
	 *            buffer of the overlay, n being the count of the buffers.
	 * - `allocator`: The last argument is the allocator for memory allocation,
	 *                default to std::allocator if not provided explicitly.
	 */
	template <class... Args>
	    requires(sizeof...(Args) ==
	             type_list_count_v<BufferTypes> +
	                 tx::allocator<tx::type_list_back_t<tx::type_list_t<Args...>>>)
	OverlayMMW(Args... args)
	    : BufferStorage(tx::allocator<tx::type_list_back_t<tx::type_list_t<Args...>>> ?
	                        arg_list_back(std::forward<Args>(args)...) :
	                        Allocator{}),
	      Base(makeBase_impl(this->buffers,
	                         std::make_index_sequence<type_list_count_v<BufferTypes>>())) {}
	// parameter parsing was used here for default parameter value of the allocator
	/**
	 * Stub: extra overlay specific parameters are not accepted (since
	 * currently no overlay have one)
	 */

	// no need to free memory because tx::BasicStorage manages it instead
	~OverlayMMW() { this->destruct_impl(); }

	OverlayMMW(const OverlayMMW& other)
	    : BufferStorage(other),
	      Base() {
		copy_impl(other);
	}
	RingBuffer(RingBuffer<T, size>&& other) : RingBufferOverlay<T>(other) {
		// just use the copy constructor of RingBufferOverlay - shallow copy
		other.null_impl();
	}
	RingBuffer& operator=(RingBuffer<T, size> other) {
		this->swap_impl(other);
		return *this;
	}

private:
};
} // namespace tx::impl