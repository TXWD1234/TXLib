// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXJson

#pragma once
#include "impl/data_utils.hpp"
#include "impl/numeric_utils.hpp"
#include "impl/packed_parted_array.hpp"
#include "impl/value_group.hpp"
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include <memory>
#include <string_view>
#include <charconv>

namespace tx {


class JsonObject;
class JsonArray;

class JsonDocument {
	template <tx::allocator>
	friend class JsonParser;
	friend JsonObject;
	friend JsonArray;

	/**
	 * This is the result of parsing produced by JsonParser.
	 * Itself does not contain logic, nor memory management. It is only
	 * responsible of displaying the data to user. 
	 */
private:
	// ################ Assets ################

	// clang-format off

	// ================ Value Type Enum ================
	enum class ValueType_impl : u32 {
		Object  = 0b0,
		Array   = 0b1,
		String  = 0b10,
		Int     = 0b11,  // actually is i64 (long long)
		Float   = 0b100, // actually is f64 (double)
		Boolean = 0b101,
		Null    = 0b110,
	};
	// clang-format on

	// ================ Value Struct ================
	// all value structs have sizeof(u32)

	struct ValueU32_impl {
		ValueType_impl type : 3;
		u32 val : 29 = 0;
	};
	struct ValueNull_impl {
		ValueType_impl type : 3 = ValueType_impl::Null;
		u32 : 29;
	};
	struct ValueBoolean_impl {
		ValueType_impl type : 3 = ValueType_impl::Boolean;
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


public:
	JsonObject root();


private:
	u8* m_data = nullptr;
	u32 m_size = 0;
	u32 m_root = 0;

	/**
	 * Structure of m_data
	 * The front of m_data is going to be a string pool storing all the strings
	 * It is going to be composed during the first phase, managed by
	 * PackedPartedArrayOverlay.
	 * 
	 * Right after the string pool is the main content of the json document.
	 * Each entry is packed tightly together in the order of their appearance.
	 */
};

// ################ Proxy Classes ################
// For user's viewing purposes

class JsonObject {
	friend JsonDocument;

public:
private:
	JsonDocument* m_doc = nullptr;
	u32 m_index = InvalidU32;

private:
	JsonObject(JsonDocument* doc, u32 index) : m_doc(doc), m_index(index) {}
};

class JsonArray {
	friend JsonDocument;

public:
private:
	JsonDocument* m_doc = nullptr;
	u32 m_index = InvalidU32;

private:
	JsonArray(JsonDocument* doc, u32 index) : m_doc(doc), m_index(index) {}
};


inline JsonObject JsonDocument::root() {
	return JsonObject{ this, m_root };
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
	using alloc64_t = typename std::allocator_traits<Allocator>::template rebind_alloc<u64>;
	using alloc64_traits = std::allocator_traits<alloc64_t>;
	using alloc32_t = typename std::allocator_traits<Allocator>::template rebind_alloc<u32>;
	using alloc32_traits = std::allocator_traits<alloc32_t>;

	[[no_unique_address]] alloc64_t m_allocator64;
	[[no_unique_address]] alloc32_t m_allocator32;

public:
	JsonParser(std::string_view str) : m_str(str) {}

	JsonDocument parse() {
		parse_impl();
		JsonDocument result;
		result.m_data = m_result;
		result.m_size = m_resultSize;
		result.m_root = m_connState.rootIndex;
		clearState_impl();
		return result;
	}

private:
	// ################ Lifetime ################

	void clearState_impl() {
		dealloc_impl();
		m_result = nullptr;
		m_token = nullptr;
		m_resultSize = 0;
		m_tokenSize = 0;
		m_connState.rootIndex = 0;
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
	tx::PackedPartedArrayOverlay<u8> m_stringPool;
	tx::PackedPartedArrayOverlay<u8>::StateStorage m_stringPoolStateStorage;

	struct TokenizerRecord;
	struct ConnectionState_impl {
		// tokenizer to compiler
		u8* tokenEnd;
		TokenizerRecord record;

		// compiler to product
		u32 rootIndex = 0;
	} m_connState;

private:
	// ################ Memory Management ################

	void alloc_impl() {
		m_resultSize = tx::divCeil(m_str.size(), sizeof(u64));
		m_tokenSize = m_resultSize;

		m_result = reinterpret_cast<u8*>(alloc64_traits::allocate(
		    m_allocator64, m_resultSize));
		m_token = reinterpret_cast<u8*>(alloc64_traits::allocate(
		    m_allocator64, m_tokenSize));
		// divide by 3 here for worse case: {"":"","":""}
		m_stringPoolMeta = alloc32_traits::allocate(
		    m_allocator32, m_str.size() / 3);

		m_resultSize *= sizeof(u64);
		m_tokenSize = m_resultSize;
	}
	void dealloc_impl() {
		alloc64_traits::deallocate(m_allocator64, reinterpret_cast<u64*>(m_token), m_tokenSize / sizeof(u64));
		alloc32_traits::deallocate(m_allocator32, reinterpret_cast<u32*>(m_stringPoolMeta), m_str.size() / 3);
	}

private:
	// ################ Static Helpers ################

	static u8* next64_impl(u8* ptr) {
		return reinterpret_cast<u8*>((reinterpret_cast<uintptr_t>(ptr) + 7) & ~(uintptr_t)0b111);
	}
	static u32 next64_impl(u32 index) {
		return (index + 7) & ~(u32)0b111;
	}

private:
	using ValueType_impl = JsonDocument::ValueType_impl;
	using ValueU32_impl = JsonDocument::ValueU32_impl;
	using ValueNull_impl = JsonDocument::ValueNull_impl;
	using ValueBoolean_impl = JsonDocument::ValueBoolean_impl;


private:
	// ################ Logic Implementation ################

	void parse_impl() {
		alloc_impl();
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
		    : m_str(parent->m_str),
		      m_stringPool(parent->m_stringPool),
		      m_token(parent->m_token),
		      m_tokenSize(parent->m_tokenSize),
		      m_state(parent->m_str.data(), parent->m_token) {
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
		// ================ Meta & State ================

		std::string_view m_str;

		tx::PackedPartedArrayOverlay<u8> m_stringPool;
		u8* m_token = nullptr;
		u32 m_tokenSize = 0;

		struct State_impl {
			const char* str = nullptr; // cursor ptr in m_str (document string)
			u8* token = nullptr; // cursor ptr in m_token (token buffer)
		} m_state;

		TokenizerRecord m_record;

	private:
		// ================ Token Managing ================
		// prefix: token

		// assume it's always aligned.
		// alignment need to be manually solve beforehand
		template <class T>
		T* tokenPush_impl() {
			u8* oldToken = m_state.token;
			m_state.token += sizeof(T);
			// <-------------------------------------------- DevNote: resize on overflow
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

		// @return id in string pool
		u32 parseString_impl() {
			m_state.str =
			    StringParser_impl(
			        m_str.substr(strIndex_impl()),
			        m_stringPool.push_back())
			        .run();
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
			root.type = ValueType_impl::Object;

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
			root.type = ValueType_impl::Array;

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
			root.type = ValueType_impl::String;
			root.val = parseString_impl();
		}
		void parseValueNumber_impl(ValueNull_impl& root) {
			tokenAlign64_impl();

			bool isFloating = parseValueNumberTestType_impl();
			if (isFloating) {
				root.type = ValueType_impl::Float;

				auto [ptr, ec] = std::from_chars(
				    m_state.str,
				    m_str.data() + m_str.size(),
				    *tokenPush_impl<f64>());
				m_state.str = ptr;
			} else {
				root.type = ValueType_impl::Int;

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
		    m_result,
		    m_resultSize,
		    m_stringPoolMeta, m_str.size() / 3,
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
		          parent->m_stringPool),
		      m_state(parent->m_result + parent->m_connState.rootIndex),
		      m_resultRoot(m_state.result) {}

		struct CompilerResult {};

		CompilerResult run() {
		}

	private:
		// ================ Meta & State ================

		struct {
			const u8* token;
			const u8* tokenEnd;
			tx::PackedPartedArrayOverlay<u8> stringPool;
		} m_source;

		struct {
			u8* result;
		} m_state;

		u8* m_resultRoot;

	private:
		using ValueType_impl = JsonDocument::ValueType_impl;
		using ValueU32_impl = JsonDocument::ValueU32_impl;
		using ValueNull_impl = JsonDocument::ValueNull_impl;
		using ValueBoolean_impl = JsonDocument::ValueBoolean_impl;
		using ValueEntry_impl = JsonDocument::ValueEntry_impl;

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

		u32 resultIndex(u8* ptr) {
			return static_cast<u32>(ptr - m_resultRoot);
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

			for (u32 i = 0; i < entryCount; i++) {
				compileEntry_impl(resultPushAt_impl<ValueEntry_impl>(metaHead));
			}

			// sorting <-----------------------------------------------------------------------------------
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
			compileValue_impl(meta.value);
		}
		// IOState: Regular
		// @return index and type of the value compiled in result buffer
		void compileValue_impl(ValueU32_impl& root) {
			ValueNull_impl header;
			std::memcpy(&header, m_source.token, sizeof(ValueNull_impl));
			root.type = header.type;

			switch (header.type) {
			case ValueType_impl::Null:
				m_source.token += sizeof(ValueNull_impl);
				break;
			case ValueType_impl::Int:
			case ValueType_impl::Float: { // same operation for both
				m_source.token += sizeof(ValueNull_impl);

				u8* aligned = next64_impl(m_state.result);
				const u8* tokenAligned = next64_impl(m_source.token);
				// by definition both i64 and f64 are 8 bytes, therefore using
				// `sizeof(i64)` works for both types
				std::memcpy(aligned, tokenAligned, sizeof(i64));
				m_source.token = tokenAligned + sizeof(i64);
				m_state.result = aligned + sizeof(i64);

				root.val = resultIndex(aligned);
			} break;
			case ValueType_impl::Boolean:
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
			case ValueType_impl::String:
				root.val = tokenConsume_impl<ValueU32_impl>().val;
				break;
			case ValueType_impl::Object:
				/**
				 * If an object / array is empty, it will not have an object in
				 * [data] partition of it's parent, but instead have a value of
				 * 0 in it's meta entry, seeing that root can never have parent
				 */
				if (tokenRead_impl<ValueU32_impl>().val == 0) {
					root.val = 0;
					m_source.token += sizeof(ValueU32_impl);
				} else {
					root.val = resultIndex(m_state.result);
					compileObject_impl();
				}
				break;
			case ValueType_impl::Array:
				if (tokenRead_impl<ValueU32_impl>().val == 0) {
					root.val = 0;
					m_source.token += sizeof(ValueU32_impl);
				} else {
					root.val = resultIndex(m_state.result);
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
			u64* newResultPtr = reinterpret_cast<u64*>(m_result);
			u32 targetSize64 = tx::divCeil(targetSize, (u32)sizeof(u64));
			tx::resize(
			    newResultPtr,
			    tx::divCeil(stringPoolDataSize, (u32)sizeof(u64)),
			    targetSize64,
			    m_resultSize / sizeof(u64),
			    m_allocator64);
			m_result = reinterpret_cast<u8*>(newResultPtr);
			m_resultSize = targetSize64 * sizeof(u64);
			m_stringPool.rebindData(m_result, m_stringPool.size_elements());
		}
	}
	void compileStringPool_impl() {
		u32 stringPoolDataSize = tx::nextAlign<u32>(m_stringPool.size_elements());
		u32* stringPoolMetaPtr = reinterpret_cast<u32*>(m_result + stringPoolDataSize);
		m_stringPool.relocateMeta(stringPoolMetaPtr, m_stringPool.size_meta());
		m_connState.rootIndex = stringPoolDataSize + m_stringPool.size_meta() * sizeof(u32);
	}

	void compile_impl() {
		compileRealloc_impl();
		compileStringPool_impl();
		Compiler_impl{ this }.run();
	}
};





} // namespace tx

/**
 * Todo:
 * - add sort in compiler
 * - add resize in tokenizer
 * - value root instead of object root
 * - escape character parser
 */