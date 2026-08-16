// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXJson

#pragma once
#include "impl/packed_parted_array.hpp"
#include "impl/value_group.hpp"
#include "tx/basic_types.hpp"
#include "tx/type_traits.hpp"
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
		      m_tokenSize(parent->m_tokenSize) {
			m_stack.reserve(16);
		}

		void run() {
			skipWhiteSpace_impl();
			stepCur_impl('{');
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
			while (WhiteSpaceGroup::contains(m_str[m_state.index])) {
				m_state.index++;
			}
		}

		char cur() const { return m_str[m_state.index]; }
		void stepCur_impl(char val) {
			if (cur() != val) {
				// DevNote: Error
			}
			m_state.index++;
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

	private:
		// ================ Meta & State ================

		JsonParser<Allocator>* m_parent;
		std::string_view m_str;

		tx::PackedPartedArrayOverlay<u8> m_stringPool;
		u8* m_token = nullptr;
		u32 m_tokenSize = 0;

		struct State_impl {
			u32 index = 0; // document index (index in m_str)
			u32 tokenIndex = 0; // index in token buffer
			bool isParsingObject = true; // flag for state machine
			// could be an enum but since there's only 2 options bool is enough
		} m_state;

	private:
		// ================ Stack ================

		struct StackObject_impl {
			bool isObject; // true is JsonObject, false is JsonArray
			u32 index; // index of the
		};
		std::vector<StackObject_impl> m_stack;

	private:
		// ================ Parsing ================
		// the actual thing
		// function prefix: parse

		// Master function of the entire state machine
		void parse_impl() {
			if (m_state.isParsingObject) {
				parseObject_impl();
			} else {
				parseArray_impl();
			}

			parseResume_impl();
		}
		void parseResume_impl() {
			if (!m_stack.empty()) {
			}
		}

		// ---------------- Structure Parsing ----------------

		// InputState: parseObjectBegin_impl called; m_state.index after last
		// control char
		// master function for json object
		void parseObject_impl() {

			parseKey_impl();

			while (true) {


				skipWhiteSpace_impl();
				if (cur() == '}') {
					m_state.index++;
					parseObjectEnd_impl();
					return;
				}
			}
		}
		void parseArray_impl() {
		}

		// InputState:  m_state.index at next char of `{`; stack in parent
		// OutputState: m_state.index at next char of `{`; stack in self
		// This function don't actually finish parsing the object, but instead
		// only update the state of the object. The entries in the object are
		// parsed after this function returned.
		// Call `parseObjectEnd_impl` to exit the object state when finished
		// parsing all entries and had reached `}`
		void parseObjectBegin_impl() {
			m_stack.push_back({ true, m_state.m_tokenIndex });
		}
		// InputState:  m_state.index at next char of `}`; stack in self
		// OutputState: m_state.index at next char of `}`; stack in parent
		void parseObjectEnd_impl() {
			m_stack.pop_back();
		}

		// // InputState:  m_state.index at next char of last control char; stack
		// //              in self
		// // OutputState: vary between parseKey_impl and parseObjectEnd_impl
		// // @return isNotEndOfObject / continue parsing object
		// // Connection between entries, decide whether to finish current object
		// // or start another entry
		// bool parseIsEndOfObject_impl() {
		// 	skipWhiteSpace_impl();
		// 	return cur() == '}';
		// }

		// InputState:  m_state.index at next char of last control char; stack
		//              in self
		// OutputState: m_state.index at first char of value; stack in self
		// Handles one entry in JsonObject. It parses the key string, and
		// advance m_state.index to the value the key is according to.
		void parseKey_impl() {
			stepCur_impl('"');

			m_state.index +=
			    StringParser_impl(
			        m_str.substr(m_state.index),
			        m_stringPool.backPartition())
			        .run();
			skipWhiteSpace_impl();
			stepCur_impl(':');
			skipWhiteSpace_impl();
		}

		// InputState:  m_state.index at first char of value; stack in self
		// OutputState:
		// @return need escape. If true then it's either an object or an array
		bool parseValueBegin_impl() {
		}
		// InputState:  m_state.index at next char of last char of value; stack
		//              in self
		// OutputState: m_state.index at next char of `,`; stack in self
		void parseValueEnd_impl() {
			skipWhiteSpace_impl();
			if (cur() == '}') {
				m_state.index++;
				parseObjectEnd_impl();
				return;
			}
			stepCur_impl(',')
		}

		// ---------------- Value Parsing ----------------
		// prefix: parseValue
		// InputState: m_state.index at first char of value; stack in self

		void parseValueString_impl() {
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