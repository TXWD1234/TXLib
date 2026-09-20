// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXJson

#pragma once
#include "impl/allocator.hpp"
#include "impl/data_utils.hpp"
#include "impl/hash_set.hpp"
#include "impl/numeric_utils.hpp"
#include "impl/packed_parted_array.hpp"
#include "impl/value_group.hpp"
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <cstddef>
#include <memory>
#include <string_view>
#include <charconv>
#include <cstring>
#include <expected>

namespace tx {

// ################ Types ################

// clang-format off

// ================ Value Type Enum ================
enum class JsonTypes : u32 {
	Object  = 0b000,
	Array   = 0b001,
	String  = 0b010,
	Int     = 0b011, // actually is i64 (long long)
	Float   = 0b100, // actually is f64 (double)
	Boolean = 0b101,
	Null    = 0b110,
};
// clang-format on

class JsonValue;
class JsonObject;
class JsonArray;

// ================ Type Traits ================

template <JsonTypes>
struct json_enum_type;
template <>
struct json_enum_type<JsonTypes::Object> {
	using type = JsonObject;
};
template <>
struct json_enum_type<JsonTypes::Array> {
	using type = JsonArray;
};
template <>
struct json_enum_type<JsonTypes::String> {
	using type = std::string_view;
};
template <>
struct json_enum_type<JsonTypes::Int> {
	using type = i64;
};
template <>
struct json_enum_type<JsonTypes::Float> {
	using type = f64;
};
template <>
struct json_enum_type<JsonTypes::Boolean> {
	using type = bool;
};
template <>
struct json_enum_type<JsonTypes::Null> {
	using type = std::nullptr_t;
};

template <JsonTypes Type>
using json_enum_type_t = typename json_enum_type<Type>::type;

// ################ Implementation Utilities ################
namespace impl::json {

using type_list =
    tx::type_list_t<
        tx::type_list_t<json_enum_type_t<JsonTypes::Object>>,
        tx::type_list_t<json_enum_type_t<JsonTypes::Array>>,
        tx::type_list_t<json_enum_type_t<JsonTypes::String>>,
        tx::type_list_t<json_enum_type_t<JsonTypes::Int>>,
        tx::type_list_t<json_enum_type_t<JsonTypes::Float>>,
        tx::type_list_t<json_enum_type_t<JsonTypes::Boolean>>,
        tx::type_list_t<json_enum_type_t<JsonTypes::Null>>>;

template <class Func>
concept invocable = []<class... Args>(tx::type_list_t<Args...>) {
	return tx::invocable_multi_or<Func, Args...>;
}(type_list{});



// ================ Value Struct ================
// all value structs have sizeof(u32)

struct ValueU32_impl {
	JsonTypes type : 3;
	u32 val : 29 = 0;
};
struct ValueNull_impl {
	JsonTypes type : 3 = JsonTypes::Null;
	u32 : 29;
};
struct ValueBoolean_impl {
	JsonTypes type : 3 = JsonTypes::Boolean;
	bool val : 1;
	u32 : 28;
};

// Json Object Entry
struct ValueEntry_impl {
	/**
		 * key:   .type = String; .val = string id of the key in StringPool
		 * value: .type = type of entry; .val = data of entry / absolute index
		 *                                      from root to the data of entry
		 * For value.val, it will be the data of entry when .type is:
		 *   String / Boolean / Null
		 *                it will be the index of the data when .type is:
		 *   Object / Array / Int / Float
		 * The index of data  is relative to the root address of the [Document]
		 * partition in the JsonDocument arena.
		 */
	ValueU32_impl key, value;
};

inline std::string_view stringPoolExtract_impl(
    u8* result, u32 index) {
	return std::string_view(
	    reinterpret_cast<const char*>(result + *impl::at<u32>(result + index)),
	    reinterpret_cast<const char*>(result + *impl::at<u32>(result + index + 1)));
}
} // namespace impl::json


// ################ Implementation ################

class JsonDocument {
	template <tx::allocator>
	friend class JsonParser;
	friend class JsonValue;
	friend class JsonObject;
	friend class JsonArray;

	/**
	 * This is the result of parsing produced by JsonParser.
	 * Itself does not contain logic, nor memory management. It is only
	 * responsible of displaying the data to user. 
	 */

public:
	template <tx::allocator Allocator = std::allocator<u8>>
	JsonDocument(std::string_view jsonString, Allocator alloc = Allocator{});
	~JsonDocument() { m_deallocFunc(); }

	JsonDocument(JsonDocument&& other)
	    : m_data(other.m_data), m_root(other.m_root),
	      m_deallocFunc(other.m_deallocFunc) {
		other.m_data = nullptr;
		other.m_root = InvalidU32;
	}
	JsonDocument& operator=(JsonDocument&& other) {
		m_deallocFunc();
		m_data = other.m_data;
		m_root = other.m_root;
		other.m_data = nullptr;
		other.m_root = InvalidU32;
		return *this;
	};
	JsonDocument(const JsonDocument&) = delete;
	JsonDocument& operator=(const JsonDocument&) = delete;

public:
	// ################ Public Interface ################

	JsonObject root() const;

	bool valid() const { return m_data && (m_root != InvalidU32); }



private:
	u8* m_data = nullptr;
	u32 m_root = InvalidU32;
	std::function<void()> m_deallocFunc;

	/**
	 * Structure of m_data
	 * The front of m_data is going to be a string pool storing all the strings
	 * It is going to be composed during the first phase, managed by
	 * PackedPartedArrayOverlay.
	 * 
	 * Right after the string pool is the main content of the json document.
	 * Each entry is packed tightly together in the order of their appearance.
	 */

	JsonDocument(
	    u8* data, u32 root,
	    std::function<void()> deallocFunc)
	    : m_data(data), m_root(root),
	      m_deallocFunc(deallocFunc) {}
};

// ################ Proxy Classes ################
// For user's viewing purposes

namespace impl {
struct JsonValueStorage {
public:
	bool valid() const { return m_data && (m_index != InvalidU32); }

protected:
	u8* m_data = nullptr;
	u32 m_index = InvalidU32;
	JsonValueStorage(u8* data, u32 index) : m_data(data), m_index(index) {}

	u32 getU32_impl() const {
		return impl::at<json::ValueU32_impl>(
		           m_data + m_index)
		    ->val;
	}

	using ValueU32_impl = impl::json::ValueU32_impl;
	using ValueNull_impl = impl::json::ValueNull_impl;
	using ValueBoolean_impl = impl::json::ValueBoolean_impl;
	using ValueEntry_impl = impl::json::ValueEntry_impl;
};
} // namespace impl


class JsonValue : public impl::JsonValueStorage {
	friend JsonDocument;
	friend JsonObject;
	friend JsonArray;

public:
	bool is(JsonTypes type) const { return getType_impl() == type; }
	JsonTypes type() const { return getType_impl(); }

	template <JsonTypes Type>
	std::expected<json_enum_type_t<Type>, JsonTypes> get() const {
		JsonTypes type = getType_impl();
		if (type != Type) return std::unexpected(type);
		return getRaw_impl<Type>();
	}
	template <JsonTypes Type>
	json_enum_type_t<Type> getUnchecked() const {
		return getRaw_impl<Type>();
	}

	template <impl::json::invocable Func>
	decltype(auto) visit(Func&& f) const;

private:
	using impl::JsonValueStorage::JsonValueStorage;

	/**
	 * The root of a JsonValue is the indexing object of the value, but not the
	 * value itself.
	 * The reason for this is because the indexing object stores the type
	 * information. 
	 */

	// ================ Memory Accessing ================

	JsonTypes getType_impl() const {
		return impl::as<ValueNull_impl>(
		           m_data + m_index)
		    .type;
	}

	template <JsonTypes Type>
	json_enum_type_t<Type> getRaw_impl() const;
};

class JsonObject : public impl::JsonValueStorage {
	friend JsonDocument;
	friend JsonValue;

public:
	u32 size() const { return getU32_impl(); }
	bool exist(std::string_view key) const {
		return findEntry_impl(key) != InvalidU32;
	}

	/**
	 * @return Invalid object if not found, no throw
	 * Instead of the find() pattern, at() can be directly called, and validity
	 * can be check with JsonValue.valid()
	 */
	JsonValue at(u32 index) const {
		if (index >= getU32_impl())
			return JsonValue(nullptr, InvalidU32);
		return JsonValue{
			m_data, static_cast<u32>(
			            m_index + sizeof(ValueU32_impl) +
			            sizeof(ValueEntry_impl) * index)
		};
	}
	/**
	 * @return Invalid object if not found, no throw
	 * Instead of the find() pattern, at() can be directly called, and validity
	 * can be check with JsonValue.valid()
	 */
	JsonValue at(std::string_view key) const {
		return at(findEntry_impl(key));
	}

	JsonValue operator[](std::string_view key) const { return at(key); }

	/**
	 * Instead of the find() pattern, at() can be directly called, and validity
	 * can be check with JsonValue.valid()
	 * find() is still provided however in case you want an index for potential
	 * usage
	 * @return index of the entry. 0xFFFFFFFF is returned if not found
	 */
	u32 find(std::string_view key) const { return findEntry_impl(key); }

private:
	using impl::JsonValueStorage::JsonValueStorage;

	std::string_view findStr_impl(std::string_view str) const { return str; }
	std::string_view findStr_impl(ValueEntry_impl entry) const {
		return impl::json::stringPoolExtract_impl(m_data, entry.key.val);
	}

	u32 findEntry_impl(std::string_view key) const {
		return tx::binarySearch(
		    key,
		    impl::at<ValueEntry_impl>(
		        m_data + m_index + sizeof(ValueU32_impl)),
		    size(), [this](auto a, auto b) -> bool {
			    return std::less<std::string_view>{}(
			        findStr_impl(a), findStr_impl(b));
		    });
	}
};

class JsonArray : public impl::JsonValueStorage {
	friend JsonDocument;
	friend JsonValue;

public:
	u32 size() const { return getU32_impl(); }

	JsonValue at(u32 index) const {
		if (index >= getU32_impl())
			return JsonValue(nullptr, InvalidU32);
		return JsonValue(
		    m_data,
		    static_cast<u32>(
		        m_index + sizeof(ValueU32_impl) +
		        sizeof(ValueU32_impl) * index));
	}

	JsonValue operator[](u32 index) const { return at(index); }

private:
	using impl::JsonValueStorage::JsonValueStorage;
};

template <>
inline JsonObject JsonValue::getRaw_impl<JsonTypes::Object>() const {
	return JsonObject{ m_data, getU32_impl() };
}
template <>
inline JsonArray JsonValue::getRaw_impl<JsonTypes::Array>() const {
	return JsonArray{ m_data, getU32_impl() };
}
template <>
inline std::string_view JsonValue::getRaw_impl<JsonTypes::String>() const {
	return impl::json::stringPoolExtract_impl(
	    m_data,
	    getU32_impl());
}
template <>
inline i64 JsonValue::getRaw_impl<JsonTypes::Int>() const {
	return *impl::at<i64>(m_data + getU32_impl());
}
template <>
inline f64 JsonValue::getRaw_impl<JsonTypes::Float>() const {
	return *impl::at<f64>(m_data + getU32_impl());
}
template <>
inline bool JsonValue::getRaw_impl<JsonTypes::Boolean>() const {
	return impl::at<ValueU32_impl>(
	           m_data + m_index)
	    ->val;
}
template <>
inline std::nullptr_t JsonValue::getRaw_impl<JsonTypes::Null>() const {
	return std::nullptr_t{};
}

template <impl::json::invocable Func>
inline decltype(auto) JsonValue::visit(Func&& f) const {
	// clang-format off
	switch (getType_impl()) {
	case JsonTypes::Object : return std::forward<Func>(f)(getRaw_impl<JsonTypes::Object >()); break;
	case JsonTypes::Array  : return std::forward<Func>(f)(getRaw_impl<JsonTypes::Array  >()); break;
	case JsonTypes::String : return std::forward<Func>(f)(getRaw_impl<JsonTypes::String >()); break;
	case JsonTypes::Int    : return std::forward<Func>(f)(getRaw_impl<JsonTypes::Int    >()); break;
	case JsonTypes::Float  : return std::forward<Func>(f)(getRaw_impl<JsonTypes::Float  >()); break;
	case JsonTypes::Boolean: return std::forward<Func>(f)(getRaw_impl<JsonTypes::Boolean>()); break;
	case JsonTypes::Null   : return std::forward<Func>(f)(getRaw_impl<JsonTypes::Null   >()); break;
	}
	// clang-format on
	std::unreachable();
}

inline JsonObject JsonDocument::root() const {
	return JsonObject{ m_data, m_root };
}


template <tx::allocator Allocator = std::allocator<u8>>
class JsonParser {
	friend JsonDocument;
	friend JsonObject;
	friend JsonArray;

private:
	/**
	 * Rebind to u64 for u64 alignment
	 */
	using alloc32_traits = tx::typed_allocator_traits<Allocator, u32>;
	using alloc64bytes_traits = tx::aligned_allocator_traits<Allocator, alignof(u64)>;

	[[no_unique_address]] Allocator m_allocator;

private:
	// ################ Interface ################

	JsonParser(std::string_view str, Allocator alloc = Allocator{})
	    : m_allocator(alloc), m_str(str) { alloc_impl(); }
	~JsonParser() { dealloc_impl(); }

	JsonDocument run() {
		parse_impl();
		return JsonDocument(
		    m_result, m_connState.rootIndex,
		    [m_allocator = m_allocator,
		     m_result = m_result,
		     m_resultSize = m_resultSize]() mutable {
			    alloc64bytes_traits::deallocate(
			        m_allocator, m_result, m_resultSize);
		    });
	}

public:
	// ################ Public Interface ################

	static JsonDocument parse(
	    std::string_view str, Allocator alloc = Allocator{}) {
		return JsonParser<Allocator>(str, alloc).run();
	}

private:
	// ################ User Parameters ################

	std::string_view m_str;

private:
	// ################ Runtime Data ################

	u8* m_result = nullptr;
	u8* m_token = nullptr;
	u32 m_resultSize = 0;
	u32 m_tokenSize = 0;

	u32* m_stringPoolMeta;
	u32 m_stringPoolMetaSize;
	tx::PackedPartedArrayOverlay<u8> m_stringPool;
	tx::PackedPartedArrayOverlay<u8>::StateStorage m_stringPoolStateStorage;

	struct TokenizerRecord;
	struct ConnectionState_impl {
		// tokenizer to compiler
		u8* tokenEnd;
		TokenizerRecord record;

		// pre-compilation process to compiler
		u32 stringPoolMetaOffset;

		// compiler to product
		u32 rootIndex = 0;
	} m_connState;

private:
	// ################ Memory Management ################

	void alloc_impl() {
		m_resultSize = m_str.size();
		m_tokenSize = m_resultSize;
		// divide m_str by 3 here for worse case: {"":"","":""}
		m_stringPoolMetaSize = m_str.size() / 3;

		m_result = alloc64bytes_traits::allocate(m_allocator, m_resultSize);
		m_token = alloc64bytes_traits::allocate(m_allocator, m_tokenSize);
		m_stringPoolMeta = alloc32_traits::allocate(
		    m_allocator, m_stringPoolMetaSize);
	}
	void dealloc_impl() {
		alloc64bytes_traits::deallocate(m_allocator, m_token, m_tokenSize);
		alloc32_traits::deallocate(m_allocator, m_stringPoolMeta, m_stringPoolMetaSize);
	}

private:
	// ################ Static Helpers ################

	static u8* next64_impl(u8* ptr) {
		return reinterpret_cast<u8*>(
		    (reinterpret_cast<uintptr_t>(ptr) + 7) & ~(uintptr_t)0b111);
	}
	static u32 next64_impl(u32 index) {
		return (index + 7) & ~(u32)0b111;
	}

	static std::string_view stringPoolExtract_impl(
	    const tx::PackedPartedArrayOverlay<u8> stringPool, u32 index) {
		auto span = stringPool[index];
		return std::string_view(
		    reinterpret_cast<const char*>(span.data()), span.size());
	}

private:
	// ################ Logic Implementation ################

	void parse_impl() {
		tokenize_impl();
		compile_impl();
	}

	// ====================================================
	// **************** Phase 1: Tokenize ****************
	// ====================================================

	struct TokenizerRecord {
		u32 oaec = 0; // objectArrayEntryCount
		u32 oac = 0; // objectArrayCount
		u32 bnnsc = 0; // booleanNullNumberStringCount
	};
	struct Tokenizer_impl {
		/**
		 * m_stringPool will not overflow capacity for both data and meta,
		 * given that the character count in all strings in impossible to be
		 * more then the actual size of the json document, and there could be
		 * at most `size / 3` string existing
		 */

		/**
		 * Terminology:
		 * InputState:  The current global state when a function is called
		 * OutputState: The current global state when a function is returned
		 */
	public:
		Tokenizer_impl(JsonParser<Allocator>* parent)
		    : m_parent(parent),
		      m_str(parent->m_str),
		      m_stringPool(parent->m_stringPool),
		      m_token(parent->m_token),
		      m_tokenSize(parent->m_tokenSize),
		      m_state(parent->m_str.data(), parent->m_token, {}),
		      m_interningBuffer(
		          allocEntry_traits::allocate(parent->m_allocator,
		                                      parent->m_stringPoolMetaSize)),
		      m_interningTable(m_interningBuffer, parent->m_stringPoolMetaSize,
		                       &m_state.interningTableState,
		                       InterningTableHash{ m_stringPool },
		                       InterningTableEqual{ m_stringPool }) {}
		~Tokenizer_impl() {
			allocEntry_traits::deallocate(
			    m_parent->m_allocator, m_interningBuffer,
			    m_parent->m_stringPoolMetaSize);
		}


		struct TokenizerResult {
			u8* token;
			u8* tokenEnd;
			u32 tokenBufferSize; // buffer size
			TokenizerRecord record;
		};

		TokenizerResult run() {
			skipWhiteSpace_impl();
			stepCur_impl('{');
			parseObject_impl(
			    *tokenPush_impl<ValueU32_impl>());
			return TokenizerResult{
				m_token, m_state.token, m_tokenSize, m_record
			};
		}

	private:
		// ================ Dedup Optimization ================

		struct InterningTableHash {
			tx::PackedPartedArrayOverlay<u8> m_stringPool;

			size_t operator()(u32 val) const {
				return std::hash<std::string_view>{}(
				    stringPoolExtract_impl(m_stringPool, val));
			}
			size_t operator()(std::string_view str) const {
				return std::hash<std::string_view>{}(str);
			}
		};
		struct InterningTableEqual {
			tx::PackedPartedArrayOverlay<u8> m_stringPool;

			bool operator()(u32 a, u32 b) const {
				return a == b;
			}
			bool operator()(u32 val, std::string_view str) const {
				return stringPoolExtract_impl(m_stringPool, val) == str;
			}
		};

		using InterningTable = tx::HashSetOverlay<
		    u32,
		    InterningTableHash,
		    InterningTableEqual>;
		using allocEntry_traits = tx::typed_allocator_traits<
		    Allocator, typename InterningTable::EntryStorage>;

	private:
		// ================ Meta & State ================

		JsonParser<Allocator>* m_parent;

		std::string_view m_str;

		tx::PackedPartedArrayOverlay<u8> m_stringPool;
		u8* m_token = nullptr;
		u32 m_tokenSize = 0;

		struct State_impl {
			const char* str = nullptr; // cursor ptr in m_str (document string)
			u8* token = nullptr; // cursor ptr in m_token (token buffer)
			typename InterningTable::StateStorage interningTableState;
			bool dedupEnabled = true;
		} m_state;

		TokenizerRecord m_record;

		typename InterningTable::EntryStorage* m_interningBuffer;

		InterningTable m_interningTable;

	private:
		using ValueU32_impl = impl::json::ValueU32_impl;
		using ValueNull_impl = impl::json::ValueNull_impl;
		using ValueBoolean_impl = impl::json::ValueBoolean_impl;
		using ValueEntry_impl = impl::json::ValueEntry_impl;

	private:
		// ================ Token Managing ================
		// prefix: token

		// assume it's always aligned.
		// alignment need to be manually solved beforehand
		template <class T>
		T* tokenPush_impl() {
			// resizing on overflow
			if (u32 tokenIndex = m_state.token - m_token;
			    tokenIndex + sizeof(T) > m_tokenSize) [[unlikely]] {
				u32 newSize = m_tokenSize * 2; // should i do a "smarter" policy
				// here? like calculate the ratio of the current buffer size and
				// the consumed m_str?
				tx::resize<u8, Allocator, alloc64bytes_traits>(
				    m_token, tokenIndex, newSize, m_tokenSize, m_parent->m_allocator);

				m_tokenSize = newSize;
				m_state.token = m_token + tokenIndex;
			}

			u8* oldToken = m_state.token;
			m_state.token += sizeof(T);

			return std::construct_at(
			    reinterpret_cast<T*>(oldToken));
		}

		void tokenAlign64_impl() {
			m_state.token = next64_impl(m_state.token);
		}

	private:
		// ================ String Parsing ================

		using WhiteSpaceGroup = ValueGroup<
		    char,
		    ' ',
		    '\t',
		    '\n',
		    '\r'>;
		// advance index to the first non-white space character
		void skipWhiteSpace_impl() {
			while (WhiteSpaceGroup::contains(cur())) {
				m_state.str++;
			}
		}

		char cur() const { return *m_state.str; }
		void stepCur_impl(char val) {
			if (cur() != val) {
				// DevNote: Error
			}
			m_state.str++;
		}
		u32 strIndex_impl() { return static_cast<u32>(m_state.str - m_str.data()); }

		inline bool isNumber_impl(char val) {
			return (val >= '0' && val <= '9') || val == '-';
		}

		struct StringParser_impl {
		public:
			using Output = typename tx::PackedPartedArrayOverlay<u8>::BackPartition;

		public:
			// @param str sub string of the json file, started with next char
			// after the first `"` of the string targeted to be parsed
			StringParser_impl(std::string_view str, Output output)
			    : m_strEnd(str.data() + str.size()), m_output(output),
			      m_state(str.data(), str.data()) {}

			// @return end ptr (next char of the last `"`)
			const char* run() {
				while (true) {
					switch (*m_state.ptr) {
					case '\\':
						pushStr_impl();
						m_state.ptr = EscapeCharacterParser_impl(
						                  m_state.ptr + 1, m_strEnd, m_output)
						                  .run();
						m_state.lastPush = m_state.ptr;
						break;
					case '"':
						pushStr_impl();
						return m_state.ptr + 1;
					default:
						m_state.ptr++;
						break;
					}
				}
			}

		private:
			struct EscapeCharacterParser_impl {
			public:
				// @param str m_str pointer from the parent scope, pointing at
				// next char after the '\' character
				EscapeCharacterParser_impl(
				    const char* str, const char* strEnd, Output output)
				    : m_str(str), m_strEnd(strEnd), m_output(output) {}

				// @return cursor ptr pointing no the next char after the
				// escape char
				const char* run() {}

			private:
				const char* m_str;
				const char* m_strEnd;
				Output m_output;
			};

		private:
			const char* m_strEnd;
			Output m_output;

			struct State_impl {
				const char* ptr;
				const char* lastPush;
			} m_state;

			void pushStr_impl() {
				m_output.push_back(m_state.lastPush, m_state.ptr);
			}
		};

		/**
		 * Deduplication (dedup) Optimization
		 * During parsing, when a encountering a string that had previously
		 * existed, instead of pushing a duplicate in string pool, the previous
		 * string will be used. An interning table (tx::HashSetOverlay) is used
		 * to dynamicly track the string ID in the string pool. The string pool
		 * ID of the corresponding string is assigned to the string entry's
		 * when the string it holds is found existing.
		 * 
		 * The bail-out policy
		 * When the interning table is full, the entire optimization will be
		 * disabled. Most likely this will not happen in normal cases, but only
		 * pathlogical worse cases which there's no major benefit handling.
		 */

		// @return id in string pool
		u32 parseString_impl() {
			m_state.str =
			    StringParser_impl(
			        m_str.substr(strIndex_impl()),
			        m_stringPool.push_back())
			        .run();
			u32 stringPoolId = m_stringPool.size_partitions() - 1;

			// dedup
			if (!m_state.dedupEnabled)
				return stringPoolId;

			u32 strInterningIndex = m_interningTable.find(
			    stringPoolExtract_impl(m_stringPool, stringPoolId));
			if (strInterningIndex != tx::InvalidU32) {
				// duplication
				m_stringPool.pop_back();
				return m_interningTable.at(strInterningIndex);
			} else {
				m_interningTable.insert(stringPoolId);
				if (m_interningTable.full()) m_state.dedupEnabled = false;
			}

			return m_stringPool.size_partitions() - 1;
		}

	private:
		// ================ Parsing ================
		// the actual thing
		// prefix: parse

		// ---------------- Structure Parsing ----------------

		// InputState:  m_state.str one after `{`
		// OutputState: m_state.str one after `}`
		// @param root object root variable, recording the number of entries
		// master function for json object
		void parseObject_impl(ValueU32_impl& root) {
			root.type = JsonTypes::Object;
			skipWhiteSpace_impl();
			if (cur() == '}') {
				m_state.str++;
				return;
			}
			stepCur_impl('"');
			root.val++;
			parseEntry_impl();
			skipWhiteSpace_impl();

			while (true) {
				switch (cur()) {
				case '}':
					// end of object
					m_state.str++;
					m_record.oaec += root.val;
					m_record.oac++;
					return;
				case ',':
					// new entry
					m_state.str++;
					root.val++;

					skipWhiteSpace_impl();
					stepCur_impl('"');
					parseEntry_impl();
					skipWhiteSpace_impl();
					break;
				default:
					// DevNote: Error
					break;
				}
			}
		}
		// InputState:  m_state.str one after `[`
		// OutputState: m_state.str one after `]`
		// @param root array root variable, recording the number of entries
		// master function for json array
		void parseArray_impl(ValueU32_impl& root) {
			root.type = JsonTypes::Array;

			skipWhiteSpace_impl();
			if (cur() == ']') {
				m_state.str++;
				return;
			}
			root.val++;
			parseValue_impl();
			skipWhiteSpace_impl();

			while (true) {
				switch (cur()) {
				case ']':
					// end of array
					m_state.str++;
					m_record.oaec += root.val;
					m_record.oac++;
					return;
				case ',':
					// new entry
					m_state.str++;
					root.val++;

					skipWhiteSpace_impl();
					parseValue_impl();
					skipWhiteSpace_impl();
					break;
				default:
					// DevNote: Error
					break;
				}
			}
		}

		// InputState:  m_state.str one after first `"`
		// OutputState: m_state.str one after value back
		// Handles one entry in JsonObject. It parses the key string, and
		// advance m_state.str to the value the key is according to.
		void parseEntry_impl() {
			// key
			tokenPush_impl<ValueU32_impl>()->val = parseString_impl();
			skipWhiteSpace_impl();
			stepCur_impl(':');
			skipWhiteSpace_impl();
			// end at first char of value

			// value
			parseValue_impl();
		}

		// ---------------- Value Parsing ----------------
		// prefix: parseValue
		// InputState:  m_state.str at first char of value
		// OutputState: m_state.str one after value back

		// master function
		void parseValue_impl() {
			switch (cur()) {
			case '[':
				parseArray_impl(*tokenPush_impl<ValueU32_impl>());
				break;
			case '{':
				parseObject_impl(*tokenPush_impl<ValueU32_impl>());
				break;
			case '"':
				parseValueString_impl(*tokenPush_impl<ValueU32_impl>());
				m_record.bnnsc++;
				break;
			default:
				if (isNumber_impl(cur())) {
					parseValueNumber_impl(*tokenPush_impl<ValueNull_impl>());
				} else if (m_str.compare(strIndex_impl(), 4, "null") == 0) {
					m_state.str += 4;
					tokenPush_impl<ValueNull_impl>();
				} else if (m_str.compare(strIndex_impl(), 4, "true") == 0) {
					m_state.str += 4;
					tokenPush_impl<ValueBoolean_impl>()->val = true;
				} else if (m_str.compare(strIndex_impl(), 5, "false") == 0) {
					m_state.str += 5;
					tokenPush_impl<ValueBoolean_impl>()->val = false;
				} else {
					// DevNote: Error
				}
				m_record.bnnsc++;
			}
		}

		void parseValueString_impl(ValueU32_impl& root) {
			root.type = JsonTypes::String;
			root.val = parseString_impl();
		}
		void parseValueNumber_impl(ValueNull_impl& root) {
			tokenAlign64_impl();

			bool isFloating = parseValueNumberTestType_impl();
			if (isFloating) {
				root.type = JsonTypes::Float;

				auto [ptr, ec] = std::from_chars(
				    m_state.str,
				    m_str.data() + m_str.size(),
				    *tokenPush_impl<f64>());
				m_state.str = ptr;
			} else {
				root.type = JsonTypes::Int;

				auto [ptr, ec] = std::from_chars(
				    m_state.str,
				    m_str.data() + m_str.size(),
				    *tokenPush_impl<i64>());
				m_state.str = ptr;
			}
		}
		using NumberInterruptingSymbolGroup = ValueGroup<
		    char,
		    ' ',
		    ',',
		    '}',
		    ']'>;
		// @return isFloating, true for floating, false for integer
		bool parseValueNumberTestType_impl() {
			const char* ptr = m_state.str;
			if (*ptr == '-') ptr++;
			while (!NumberInterruptingSymbolGroup::contains(*ptr)) {
				if (*ptr == '.' || *ptr == 'e' || *ptr == 'E')
					return true;
				ptr++;
			}
			return false;
		}
	};

	void tokenize_impl() {
		m_stringPool = tx::PackedPartedArrayOverlay<u8>(
		    m_result, m_resultSize,
		    m_stringPoolMeta, m_stringPoolMetaSize,
		    &m_stringPoolStateStorage);

		auto [token, tokenEnd, tokenBufferSize, record] =
		    Tokenizer_impl{ this }.run();
		m_token = token;
		m_tokenSize = tokenBufferSize;
		m_connState.tokenEnd = tokenEnd;
		m_connState.record = record;
	}

	// ==================================================
	// **************** Phase 2: Compile ****************
	// ==================================================

	struct Compiler_impl {
		/**
		 * The data structure of the compiled JsonDocument arena:
		 * ```
		 * [StringPoolData][StringPoolMeta][Document]
		 * ```
		 * Inside [Document], there are 2 recurse structures: Object and Array
		 * Both of them have the same structure of: (Unit: byte)
		 * ```
		 * [root][meta][data]
		 * ^     ^     ^
		 * 0     4     (potential padding of 4) sizeof(meta) * entryCount
		 * ```
		 * Where both [root] and [data] are similar, only meta differs:
		 * [root]:
		 * ValueU32_impl object with .val = entryCount, .type = Object / Array
		 * [meta]:
		 * - Object:
		 * [meta] is an array of ValueEntry_impl.
		 * - Array:
		 * [meta] is an array of ValueU32_impl, which only record the location
		 * of the entries, exluding the `key` from ValueEntry_impl
		 * [data]:
		 * The actual data of entries (exluding those entries that can already
		 * fit in [meta]), which can encompass sub-structures of other Object
		 * or Array.
		 */
	public:
		Compiler_impl(JsonParser<Allocator>* parent)
		    : m_source(
		          parent->m_token,
		          parent->m_connState.tokenEnd,
		          parent->m_connState.stringPoolMetaOffset),
		      m_state(parent->m_result + parent->m_connState.rootIndex),
		      m_result(parent->m_result) {}

		struct CompilerResult {};

		CompilerResult run() {
		}

	private:
		// ================ Meta & State ================

		struct {
			const u8* token;
			const u8* tokenEnd;
			u32 stringPoolMetaOffset;
		} m_source;

		struct {
			u8* result;
		} m_state;

		u8* m_result;

	private:
		using ValueU32_impl = impl::json::ValueU32_impl;
		using ValueNull_impl = impl::json::ValueNull_impl;
		using ValueBoolean_impl = impl::json::ValueBoolean_impl;
		using ValueEntry_impl = impl::json::ValueEntry_impl;

	private:
		// ================ Helper Functions ================

		// ---------------- Result Placement Managing ----------------
		// the 2 token managing functions are directly copied from tokenizer,
		// but removed the resizing branch (therefore cannot be generalized)

		// assume it's always aligned.
		// alignment need to be manually solve beforehand
		template <class T, class... Args>
		T* resultPush_impl(Args&&... args) {
			u8* old = m_state.result;
			m_state.result += sizeof(T);
			return std::construct_at(
			    reinterpret_cast<T*>(old), std::forward<Args>(args)...);
		}
		template <class T, class... Args>
		T* resultPushAt_impl(u8*& ptr, Args&&... args) {
			u8* old = ptr;
			ptr += sizeof(T);
			return std::construct_at(
			    reinterpret_cast<T*>(old), std::forward<Args>(args)...);
		}

		void resultAlign64_impl() {
			m_state.result = next64_impl(m_state.result);
		}

		// assume it's always aligned
		template <class T>
		u8* resultAdvance_impl(u32 count) {
			u8* old = m_state.result;
			m_state.result += sizeof(T) * count;
			return old;
		}

		template <class T>
		const T& tokenRead_impl() {
			return *impl::at<T>(m_source.token);
		}
		template <class T>
		const T& tokenConsume_impl() {
			const u8* old = m_source.token;
			m_source.token += sizeof(T);
			return *impl::at<T>(old);
		}
		// template <class T>
		// T& tokenAt_impl(u8* ptr) {
		// 	return *impl::at<T>(ptr);
		// }

		u32 resultIndex_impl(u8* ptr) {
			return static_cast<u32>(ptr - m_result);
		}
		// turn string pool id (from tokenizer) to physical offset in m_result
		// unit: index: u32 / string pool id -> ret: u8
		u32 stringPoolIndex_impl(u32 index) {
			return index * sizeof(u32) + m_source.stringPoolMetaOffset;
		}

	private:
		// ================ Compiling ================
		// parsing the token buffer and compile it into result buffer
		// prefix: compile
		// Terminology:
		// - IState / OState: InputState / OutputState
		// - "IOState: Regular": IState: m_source.token at root;
		//                       OState: m_source.token at next root.
		// comment: this is one of the most satisfying code i've even written

		/**
		 * Because the source: token buffer was generated by internal
		 * implementation, it is guaranteed to not have malformed structure,
		 * therefore no security check is required here.
		 */

		// IState: m_source.token at object root
		// OState: m_source.token at root of next entry
		void compileObject_impl() {
			const u32 entryCount = resultPush_impl<ValueU32_impl>(
			                           tokenConsume_impl<ValueU32_impl>())
			                           ->val;
			u8* metaHead = resultAdvance_impl<ValueEntry_impl>(entryCount);
			u8* metaBegin = metaHead;

			for (u32 i = 0; i < entryCount; i++) {
				compileEntry_impl(resultPushAt_impl<ValueEntry_impl>(metaHead));
			}

			// sorting
			std::sort(
			    impl::at<ValueEntry_impl>(metaBegin),
			    impl::at<ValueEntry_impl>(metaHead),
			    [&](const ValueEntry_impl& a, const ValueEntry_impl& b) {
				    return std::less<std::string_view>{}(
				        impl::json::stringPoolExtract_impl(m_result, a.key.val),
				        impl::json::stringPoolExtract_impl(m_result, b.key.val));
			    });
		}

		// IState: m_source.token at object root
		// OState: m_source.token at root of next entry
		void compileArray_impl() {
			const u32 entryCount = resultPush_impl<ValueU32_impl>(
			                           tokenConsume_impl<ValueU32_impl>())
			                           ->val;
			u8* metaHead = resultAdvance_impl<ValueU32_impl>(entryCount);

			for (u32 i = 0; i < entryCount; i++) {
				compileValue_impl(resultPushAt_impl<ValueU32_impl>(metaHead));
			}
		}

		// IOState: Regular
		// @param meta meta data object at object root for this entry
		void compileEntry_impl(ValueEntry_impl& meta) {
			meta.key = tokenConsume_impl<ValueU32_impl>();
			meta.key.val = stringPoolIndex_impl(meta.key.val);
			compileValue_impl(meta.value);
		}
		// IOState: Regular
		// @return index and type of the value compiled in result buffer
		void compileValue_impl(ValueU32_impl& root) {
			ValueNull_impl header;
			std::memcpy(&header, m_source.token, sizeof(ValueNull_impl));
			root.type = header.type;

			switch (header.type) {
			case JsonTypes::Null:
				m_source.token += sizeof(ValueNull_impl);
				break;
			case JsonTypes::Int:
			case JsonTypes::Float: { // same operation for both
				m_source.token += sizeof(ValueNull_impl);

				u8* aligned = next64_impl(m_state.result);
				const u8* tokenAligned = next64_impl(m_source.token);
				// by definition both i64 and f64 are 8 bytes, therefore using
				// `sizeof(i64)` works for both types
				std::memcpy(aligned, tokenAligned, sizeof(i64));
				m_source.token = tokenAligned + sizeof(i64);
				m_state.result = aligned + sizeof(i64);

				root.val = resultIndex_impl(aligned);
			} break;
			case JsonTypes::Boolean:
				/**
				 * This is kind of an inconsistency: the tokenizer enforce
				 * boolean to be an unique type, but here just uses the u32 to
				 * store the boolean directly. But doing another cast or
				 * something here is just way too complicated and not worth the
				 * effort, seeing that there's no actual performance difference
				 * between 2 approaches.
				 */
				root.val = tokenConsume_impl<ValueBoolean_impl>().val;
				break;
			case JsonTypes::String:
				root.val = stringPoolIndex_impl(tokenConsume_impl<ValueU32_impl>().val);
				break;
			case JsonTypes::Object:
				/**
				 * If an object / array is empty, it will not have an object in
				 * [data] partition of it's parent, but instead have a value of
				 * 0 in it's meta entry, seeing that root can never have parent
				 */
				if (tokenRead_impl<ValueU32_impl>().val == 0) {
					root.val = 0;
					m_source.token += sizeof(ValueU32_impl);
				} else {
					root.val = resultIndex_impl(m_state.result);
					compileObject_impl();
				}
				break;
			case JsonTypes::Array:
				if (tokenRead_impl<ValueU32_impl>().val == 0) {
					root.val = 0;
					m_source.token += sizeof(ValueU32_impl);
				} else {
					root.val = resultIndex_impl(m_state.result);
					compileArray_impl();
				}
				break;
			}
		}
	};

	// reallocate m_result
	void compileRealloc_impl() {
		/**
		 * The size of the final m_result can be precisely calculated with
		 * records recorded during the tokenlizing phase.
		 * The final document data size can be derived from the token buffer
		 * size.
		 * In the 7 types of objects:
		 * - Object and Array each require one more u32 for each entry of them
		 *   for their entry meta data. Plus one extra u32 for each Object /
		 *   Array for potential padding between [meta] and [data] in their
		 *   structure.
		 * - Boolean, Null, Number and String each will release one u32. It
		 *   used to store the type of the value, but now in the final
		 *   compilation, the type is stored in the meta data.
		 * Plus the string pool data and meta data, which are stored at the
		 * front of the buffer.
		 * 
		 * note: all size calculated in unit of byte (sizeof(u8))
		 */

		u32 stringPoolDataSize = tx::nextAlign<u32>(m_stringPool.size_elements());
		u32 stringPoolMetaSize = m_stringPool.size_meta() * sizeof(u32);
		u32 tokenDataSize = static_cast<u32>(m_connState.tokenEnd - m_token);
		u32 subSize = m_connState.record.bnnsc * sizeof(u32);
		u32 addSize = (m_connState.record.oaec + m_connState.record.oac) * sizeof(u32);

		u32 targetSize =
		    stringPoolDataSize +
		    stringPoolMetaSize +
		    tokenDataSize -
		    subSize + addSize;

		if (targetSize > m_resultSize) {
			// realloc
			tx::resize<u8, Allocator, alloc64bytes_traits>(
			    m_result,
			    stringPoolDataSize,
			    targetSize,
			    m_resultSize,
			    m_allocator);
			m_resultSize = targetSize;
		}
	}
	void compileStringPool_impl() {
		u32 stringPoolDataSize = tx::nextAlign<u32>(m_stringPool.size_elements());
		u32* stringPoolMetaPtr = reinterpret_cast<u32*>(m_result + stringPoolDataSize);
		tx::uninitialized_relocate(
		    m_stringPoolMeta, m_stringPoolMeta + m_stringPool.size_meta(), stringPoolMetaPtr);
		// potentially obsolete since compiler don't need a string pool object
		// anymore
		m_stringPool = tx::PackedPartedArrayOverlay<u8>::fromExistingState(
		    m_result, m_stringPool.size_elements(),
		    stringPoolMetaPtr, m_stringPool.size_meta(),
		    &m_stringPoolStateStorage);
		m_connState.rootIndex = stringPoolDataSize + m_stringPool.size_meta() * sizeof(u32);
		m_connState.stringPoolMetaOffset = stringPoolDataSize;
	}

	void compile_impl() {
		compileRealloc_impl();
		compileStringPool_impl();
		Compiler_impl{ this }.run();
	}
};

template <tx::allocator Allocator>
JsonDocument::JsonDocument(
    std::string_view jsonString, Allocator alloc)
    : JsonDocument(JsonParser<Allocator>::parse(jsonString, alloc)) {}





} // namespace tx

/**
 * TODO:
 * Use PackedPartedArrayOverlayInlined and HashSet
 */


/**
 * Todo:
 * - value root instead of object root
 * - escape character parser
 */