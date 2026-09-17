// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXUtility

#pragma once
#include "tx/basic_types.hpp"
#include <type_traits>
#include <concepts>
#include <iterator>
#include <tuple>
#include <utility>

namespace tx {

template <class T>
    requires std::is_enum_v<T>
inline constexpr std::underlying_type_t<T>
enumval(T in) noexcept {
	return static_cast<std::underlying_type_t<T>>(in);
}

template <class>
inline constexpr bool false_v = false;


// ================ ########### ================
// **************** Type Traits ****************
// ================ ########### ================

// Conditional Presence
class Nothing {};

// propagate constness from type From to type To
template <class From, class To>
using const_propagate = std::conditional_t<
    std::is_const_v<std::remove_reference_t<From>>, const To, To>;

// ################ Type List ################
/**
 * Key Insights:
 * - **A type_list is still a type.**
 * - There can be multiple variadic template lists in partial specialization's
 *   template
 */

template <class... Args>
struct type_list_t {
	using type_list_tag = void;
	template <template <class...> class Template>
	using apply = Template<Args...>;
};
// type_list
template <class T>
concept type_list = requires { typename T::type_list_tag; };

template <tx::type_list List>
struct type_list_count;
template <class... Args>
struct type_list_count<type_list_t<Args...>> {
	static constexpr size_t value = sizeof...(Args);
};
template <tx::type_list List>
constexpr size_t type_list_count_v = type_list_count<List>::value;

template <tx::type_list List>
    requires(type_list_count<List>::value > 0)
struct type_list_front;
template <class T, class... Args>
struct type_list_front<type_list_t<T, Args...>> {
	using type = T;
};
template <tx::type_list List>
using type_list_front_t = typename type_list_front<List>::type;

template <tx::type_list, class...>
struct type_list_append;
template <class... Args, class... Ts>
struct type_list_append<type_list_t<Args...>, Ts...> {
	using type = type_list_t<Args..., Ts...>;
};
template <tx::type_list, class...>
struct type_list_append_front;
template <class... Args, class... Ts>
struct type_list_append_front<type_list_t<Args...>, Ts...> {
	using type = type_list_t<Ts..., Args...>;
};
template <tx::type_list List, class... Ts>
using type_list_append_t = typename type_list_append<List, Ts...>::type;
template <tx::type_list List, class... Ts>
using type_list_append_front_t = typename type_list_append_front<List, Ts...>::type;

template <tx::type_list>
struct type_list_flatten;
template <>
struct type_list_flatten<type_list_t<>> {
	using type = type_list_t<>;
};
template <class... Args>
struct type_list_flatten<type_list_t<type_list_t<Args...>>> {
	using type = type_list_t<Args...>;
};
template <class... Args, tx::type_list... Others>
struct type_list_flatten<type_list_t<type_list_t<Args...>, Others...>> {
	using type = typename type_list_append_front<
	    typename type_list_flatten<type_list_t<Others...>>::type, Args...>::type;
};
template <tx::type_list List>
using type_list_flatten_t = typename type_list_flatten<List>::type;

template <tx::type_list... Lists>
struct type_list_combine {
	using type = typename type_list_flatten<type_list_t<Lists...>>::type;
};
template <tx::type_list... Lists>
using type_list_combine_t = typename type_list_combine<Lists...>::type;

template <tx::type_list List, size_t Begin, size_t End>
    requires(Begin <= End && End <= type_list_count<List>::value)
struct type_list_sublist;
template <size_t Begin, size_t End, class T, class... Args>
    requires(Begin > 0)
struct type_list_sublist<type_list_t<T, Args...>, Begin, End> {
	using type = typename type_list_sublist<type_list_t<Args...>, Begin - 1, End - 1>::type;
};
template <size_t End, class T, class... Args>
    requires(End > 0)
struct type_list_sublist<type_list_t<T, Args...>, 0, End> {
	using type = typename type_list_append_front<
	    typename type_list_sublist<type_list_t<Args...>, 0, End - 1>::type, T>::type;
};
template <class... Args>
struct type_list_sublist<type_list_t<Args...>, 0, 0> {
	using type = type_list_t<>;
};
template <tx::type_list List, size_t Begin, size_t End>
using type_list_sublist_t = typename type_list_sublist<List, Begin, End>::type;

template <tx::type_list List, size_t Index>
    requires(Index < type_list_count<List>::value)
struct type_list_at {
	using type = typename type_list_front<
	    typename type_list_sublist<
	        List, Index, Index + 1>::type>::type;
};
template <tx::type_list List, size_t Index>
using type_list_at_t = typename type_list_at<List, Index>::type;

template <tx::type_list List>
    requires(type_list_count<List>::value > 0)
struct type_list_back {
	using type = typename type_list_at<
	    List, type_list_count<List>::value - 1>::type;
};
template <tx::type_list List>
using type_list_back_t = typename type_list_back<List>::type;

template <std::size_t Index, class... Args>
    requires(Index < sizeof...(Args))
constexpr decltype(auto) arg_list_at(Args&&... args) {
	return std::get<Index>(std::forward_as_tuple(std::forward<Args>(args)...));
}
template <class... Args>
constexpr decltype(auto) arg_list_front(Args&&... args) {
	return arg_list_at<0>(std::forward<Args>(args)...);
}
template <class... Args>
constexpr decltype(auto) arg_list_back(Args&&... args) {
	return arg_list_at<sizeof...(Args) - 1>(std::forward<Args>(args)...);
}









// ================ ######## ================
// **************** Concepts ****************
// ================ ######## ================

template <class T, template <class...> class Trait, class... Args>
concept satisfies = Trait<T, Args...>::value;

// is instantiatin of
template <typename T, template <typename...> class Template>
struct is_instantiation_of : std::false_type {};
template <template <typename...> class Template, typename... Args>
struct is_instantiation_of<Template<Args...>, Template> : std::true_type {};
template <typename T, template <typename...> class Template>
concept instantiation_of = is_instantiation_of<T, Template>::value;

// any_of
template <class T, class... Ts>
concept any_of = (std::same_as<T, Ts> || ...);

// numeric
template <class T>
concept numeric = (std::integral<T> || std::floating_point<T>) &&
                  !std::same_as<T, bool>;

// byte_like
template <typename T>
concept byte_like = sizeof(T) == 1 &&
                    (std::integral<T> || std::same_as<T, std::byte>);

// transparent
template <typename T>
concept transparent = requires { typename T::is_transparent; };

// std add on

// std::invocable with return type
template <class Func, class RetT, class... Args>
concept invocable_r = std::is_invocable_r_v<RetT, Func, Args...>;

// iterator with value type
template <class It, class T>
concept iterator_value_type =
    std::same_as<std::remove_cv_t<std::iter_value_t<It>>, T>;
template <class It, class T>
concept input_iterator_value_type =
    std::input_iterator<It> &&
    iterator_value_type<It, T>;
template <class It, class T>
concept forward_iterator_value_type =
    std::forward_iterator<It> &&
    iterator_value_type<It, T>;
template <class It, class T>
concept bidirectional_iterator_value_type =
    std::bidirectional_iterator<It> &&
    iterator_value_type<It, T>;
template <class It, class T>
concept random_access_iterator_value_type =
    std::random_access_iterator<It> &&
    iterator_value_type<It, T>;
template <class It, class T>
concept contiguous_iterator_value_type =
    std::contiguous_iterator<It> &&
    iterator_value_type<It, T>;


enum class TypeEnum : u32 {
	Float,
	Int,
	U8,
	U32,
	Vec2,
	Coord
};

template <class T>
struct type_enum {
	static_assert(false_v<T>, "tx::type_enum: Unsupported type.");
};
template <>
struct type_enum<float> {
	static constexpr TypeEnum value = TypeEnum::Float;
};
template <>
struct type_enum<int> {
	static constexpr TypeEnum value = TypeEnum::Int;
};
template <>
struct type_enum<u8> {
	static constexpr TypeEnum value = TypeEnum::U8;
};
template <>
struct type_enum<u32> {
	static constexpr TypeEnum value = TypeEnum::U32;
};

template <class T>
inline constexpr TypeEnum type_enum_v = type_enum<T>::value;


template <TypeEnum T>
struct enum_type;

template <>
struct enum_type<TypeEnum::Float> {
	using type = float;
};
template <>
struct enum_type<TypeEnum::Int> {
	using type = int;
};
template <>
struct enum_type<TypeEnum::U8> {
	using type = u8;
};
template <>
struct enum_type<TypeEnum::U32> {
	using type = u32;
};

template <TypeEnum T>
using enum_type_t = typename enum_type<T>::type;

}; // namespace tx