// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXJson

#pragma once
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
#include "impl/numeric_utils.hpp"
#include "impl/packed_parted_array.hpp"
#include "impl/value_group.hpp"
#include <memory>
#include <string_view>
#include <vector>

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
private:
	/**
	 * Rebind to u64 for u64 alignment
	 */
	using alloc64_t = typename std::allocator_traits<Allocator>::template rebind_alloc<u64>;
	using alloc64_traits = std::allocator_traits<alloc64_t>;
	using alloc32_t = typename std::allocator_traits<Allocator>::template rebind_alloc<u32>;
	using alloc32_traits = std::allocator_traits<alloc64_t>;

	[[no_unique_address]] alloc64_t m_allocator64;
	[[no_unique_address]] alloc32_t m_allocator32;

public:
	JsonParser(std::string_view str) : m_str(str) {}

	JsonDocument parse() const {
		parse_impl();
		JsonDocument result;
		result.m_data = m_result;
		result.m_size = m_resultSize;
		result.m_root = m_rootIndex;
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
		m_rootIndex = 0;
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

	u32 m_rootIndex = 0;


private:
	// ################ Memory Management ################

	void alloc_impl() {
		m_resultSize = (m_str.size() + sizeof(u64) - 1) / sizeof(u64);
		m_tokenSize = m_resultSize;

		m_result = reinterpret_cast<u8*>(alloc64_traits::allocate(
		    m_allocator64, m_resultSize));
		m_token = reinterpret_cast<u8*>(alloc64_traits::allocate(
		    m_allocator64, m_tokenSize));
		// divide by 3 here for worse case: {"":"","":""}
		m_stringPoolMeta = alloc32_traits::allocate(
		    m_allocator32, m_str.size() / 3);
	}
	void dealloc_impl() {
		alloc64_traits::deallocate(m_allocator64, m_token, m_tokenSize);
		alloc32_traits::deallocate(m_allocator32, m_stringPoolMeta, m_str.size() / 3);
	}

private:
	// ################ Static Helpers ################

	template <class T>
	static T* at_impl(u8* ptr, u32 index) {
		return std::launder(reinterpret_cast<T*>(ptr + index));
	}

	static u8* next64_impl(u8* ptr) {
		return reinterpret_cast<u8*>((reinterpret_cast<uintptr_t>(ptr) + 7) & ~(uintptr_t)0b111);
	}
	static u32 next64_impl(u32 index) {
		return (index + 7) & ~(u32)0b111;
	}



private:
	// ################ Logic Implementation ################

	void parse_impl() {
		alloc_impl();
		tokenlize_impl();
		compile_impl();
	}

	// ====================================================
	// **************** Stage 1: Tokenlize ****************
	// ====================================================

	struct Tokenlizer_impl {
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
		 * 
		 * Exit:   Expand a new object / array from the middle of parsing an
		 *         object / array, and interupt the current parsing progress
		 * Resume: After parsed the interupting object / array, back to parsing
		 *         the object / array that was being parsed before
		 */
	public:
		Tokenlizer_impl(JsonParser<Allocator>* parent)
		    : m_parent(parent),
		      m_str(parent->m_str),
		      m_stringPool(parent->m_stringPool),
		      m_token(parent->m_token),
		      m_tokenSize(parent->m_tokenSize),
		      m_state({ parent->m_str.data(), parent->m_token }) {
		}

		void run() {
			skipWhiteSpace_impl();
			stepCur_impl('{');
			parseObject_impl(
			    *tokenPush_impl<u32>());
		}

	private:
		// ================ Meta & State ================

		JsonParser<Allocator>* m_parent;
		std::string_view m_str;

		tx::PackedPartedArrayOverlay<u8> m_stringPool;
		u8* m_token = nullptr;
		u32 m_tokenSize = 0;

		struct State_impl {
			u8* str = nullptr; // cursor ptr in m_str (document string)
			u8* token = nullptr; // cursor ptr in m_token (token buffer)
		} m_state;

	private:
		// ================ Token Managing ================
		// prefix: token

		// assume it's always aligned
		// alignment need to be manually solve beforehand
		template <class T>
		T* tokenPush_impl() {
			u8* oldToken = m_state.token;
			m_state.token += sizeof(T);
			return std::construct_at(
			    reinterpret_cast<T>(oldToken));
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
			return (val >= '0' && val < '9') || val == '-';
		}

		struct StringParser_impl {
		public:
			using Output = typename tx::PackedPartedArrayOverlay<u8>::BackPartition;

		public:
			// @param str sub string of the json file, started with next char
			// after the first `"` of the string targeted to be parsed
			StringParser_impl(std::string_view str, Output output)
			    : m_str(str), m_output(output) {}

			// @return end index (next index of the last `"`)
			u32 run() {
			}

		private:
			std::string_view m_str;
			Output m_output;
		};

		// @return id in string pool
		u32 parseString_impl() {
			m_state.str +=
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
		void parseObject_impl(u32& root) {
			skipWhiteSpace_impl();
			if (cur() == '}') return;
			stepCur_impl('"');
			parseEntry_impl();
			skipWhiteSpace_impl();

			while (true) {
				switch (cur()) {
				case '}':
					// end of object
					m_state.str++;
					return;
				case ',':
					// new entry
					m_state.str++;
					root++;

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
		void parseArray_impl(u32& root) {
		}

		// InputState:  m_state.str one after first `"`
		// OutputState: m_state.str one after value back
		// Handles one entry in JsonObject. It parses the key string, and
		// advance m_state.str to the value the key is according to.
		void parseEntry_impl() {
			// key
			*tokenPush_impl<u32>() = parseString_impl(); // <---------------------
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
		}


		void parseValueString_impl(u32& root) {
		}
		void parseValueNumber_impl(u32& root) {
		}
	};

	void tokenlize_impl() {
		m_stringPool = tx::PackedPartedArrayOverlay<u8>(
		    m_result, m_resultSize, m_stringPoolMeta, m_str.size() / 3, &m_stringPoolStateStorage);

		Tokenlizer_impl tokenlizer{ this };
		tokenlizer.run();
	}


	// ==================================================
	// **************** Stage 2: Compile ****************
	// ==================================================

	void compile_impl() {
	}
};





} // namespace tx