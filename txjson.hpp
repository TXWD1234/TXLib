// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// File: TXLib_Json

#pragma once
#include "txlib.hpp"
#include "txmap.hpp"

namespace tx {

	class JsonParser;
	enum class JsonType {
		Boolean,
		Int,
		Float,
		String,
		Array,
		JsonObject
	};
	class JsonValue;
	using JsonArray     = vector<JsonValue>;
	using JsonMap       = KVMap<      string, JsonValue>;
	using JsonMapHandle = KVMapHandle<string, JsonValue>;
	using JsonPair      = KVPair<     string, JsonValue>;

	class JsonObject {
		friend class JsonParser;
	public:
		using iterator = JsonMap::      iterator;
		using const_iterator   = JsonMap::const_iterator;
		//JsonObject(JsonObject* in_parent = nullptr) : parent(in_parent) {}

		inline       JsonValue& operator[](const string& key)       { return this->members.at(key); }
		inline const JsonValue& operator[](const string& key) const { return this->members.at(key); }

		inline       JsonPair& atIndex(int index)       { return this->members.atIndex(index); }
		inline const JsonPair& atIndex(int index) const { return this->members.atIndex(index); }

		inline int  size() const { return this->members.size(); }
		inline bool empty() const { return this->members.empty(); }

		inline bool exist(const string& key) const { return members.exist(key); }

		template<class T>
		inline const T& getOr(const string& key, const T& fallback) const;

		inline iterator       begin()       { return members.begin(); }
		inline const_iterator begin() const { return members.begin(); }
		inline iterator       end()         { return members.end(); }
		inline const_iterator end()   const { return members.end(); }
	private:
		JsonMap members;
		
		void validate() {
			members.validate();
			/*for (KVPair<string, JsonValue>& i : members) {
				if (i.v().is<JsonObject>()) {
					i.v().get<JsonObject>().validate();
				}
			}*/
		}


		//JsonObject* parent = nullptr;

	};
	class JsonValue {
		using JT = std::variant<
			bool,
			int,
			float,
			string,
			JsonArray,
			JsonObject
		>;
		friend class JsonParser;
	public:
		JsonValue() {}
		JsonValue(const JT& in_var) :
			m_var(in_var) {
		}

		template<class T>
		inline bool is() const { return std::holds_alternative<T>(this->m_var); }
		inline JsonType type() const {
			return std::visit([](auto&& v) -> JsonType {
				using T = std::decay_t<decltype(v)>;
				if      constexpr (std::is_same_v<T, bool>)       return JsonType::Boolean;
				else if constexpr (std::is_same_v<T, int>)        return JsonType::Int;
				else if constexpr (std::is_same_v<T, float>)      return JsonType::Float;
				else if constexpr (std::is_same_v<T, string>)     return JsonType::String;
				else if constexpr (std::is_same_v<T, JsonArray>)  return JsonType::Array;
				else if constexpr (std::is_same_v<T, JsonObject>) return JsonType::JsonObject;
				else static_assert(sizeof(T) == 0, "Unhandled JsonValue type");
				}, m_var);
		}
		template<class T>
		inline T& get() { return std::get<T>(this->m_var); }
		template<class T>
		inline const T& get() const { return std::get<T>(this->m_var); }
		
		inline explicit operator bool() const { return std::get<bool>(this->m_var); }

		inline       JsonValue& operator[](int index)               { return std::get<JsonArray> (this->m_var)[index]; }
		inline       JsonValue& operator[](const string& key)       { return std::get<JsonObject>(this->m_var)[key]; }
		inline const JsonValue& operator[](int index)         const { return std::get<JsonArray> (this->m_var)[index]; }
		inline const JsonValue& operator[](const string& key) const { return std::get<JsonObject>(this->m_var)[key]; }

		inline JsonValue& operator=(bool              val) { this->m_var =           val;  return *this; }
		inline JsonValue& operator=(int               val) { this->m_var =           val;  return *this; }
		inline JsonValue& operator=(float             val) { this->m_var =           val;  return *this; }
		inline JsonValue& operator=(const string&     val) { this->m_var =           val;  return *this; }
		inline JsonValue& operator=(string&&          val) { this->m_var = std::move(val); return *this; }
		inline JsonValue& operator=(const JsonArray&  val) { this->m_var =           val;  return *this; }
		inline JsonValue& operator=(JsonArray&&       val) { this->m_var = std::move(val); return *this; }
		inline JsonValue& operator=(const JsonObject& val) { this->m_var =           val;  return *this; }


	private:
		JT m_var;
	};

	template<class T>
	inline const T& JsonObject::getOr(const string& key, const T& fallback) const {
			return (exist(key) ? members.at(key).get<T>() : fallback);
	}



	inline bool isNumber(char in) {
		return in >= 48 && in < 58 || in == '-';
	}
	inline bool isTrue(char in) {
		return in == 'T' || in == 't';
	}
	inline bool isFalse(char in) {
		return in == 'F' || in == 'f';
	}
	inline bool isTrueFalse(char in) {
		return isTrue(in) || isFalse(in);
	}




	class JsonParser {
	public:
		void parse(const string& str, JsonObject& root) {
			int index = 0;
			this->end = str.data() + str.size();
			parseObject_impl(str, root, index);
		}
	private:
		const char* end = nullptr;


		void parseObject_impl(const string& str, JsonObject& root, int& index) { // index is where the { of the object is in the entire string
			++index;
			KVMap<string, JsonValue>& rootmap = root.members;

			while (1) {
				index = str.find_first_not_of("\n\t\r ,", index);
				if (index == string::npos) throw_impl(index);
				if (str[index] == '}') break;
				if (str[index] != '"') throw_impl(index);
				_parseKeyValue(str, root, index);

			}

			rootmap.validate();
			++index;
		}
		void _parseKeyValue(const string& str, JsonObject& root, int& index) { // index is where the " of the key is in the entire string
			JsonMap& rootmap = root.members;
			// determind key
			JsonMapHandle hValue = rootmap.insertMulti(parseString__impl(str, index));

			index = str.find_first_not_of(": ", index);
			if (index == string::npos) throw_impl(index);
			_parseValue(str, hValue.get(), index);
		}
		void _parseValue(const string& str, JsonValue& value, int& index) { // index is the first character of the value
			switch (str[index]) {
			case '"':
				parseString_impl(str, value, index);
				break;
			case '[':
				parseArray_impl(str, value, index);
				break;
			case '{':
				value = JsonObject{};
				parseObject_impl(str, value.get<JsonObject>(), index);
				break;
			default:
				if (isNumber(str[index])) {
					parseNumber_impl(str, value, index);
				}
				else if (str.compare(index, 4, "true") == 0) {
					value = true;
					index += 4;
				}
				else if (str.compare(index, 5, "false") == 0) {
					value = false;
					index += 5;
				}
				else {
					throw_impl(index);
				}
			}
			// after this function returns, index should be at the index after the value, which should be a comma: ,
		}
		void parseNumber_impl(const string& str, JsonValue& value, int& index) { // index is the first character of the number
			// determind is int or float
			int dotIndex = str.find('.', index),
				endIndex = str.find_first_of("}, ]", index);
			if (dotIndex == string::npos || dotIndex > endIndex) {
				parseInt_impl(str, value, index);
			}
			else {
				parseFloat_impl(str, value, index);
			}
			/*int tempIndex = index;
			while (isNumber(str[index])) ++index;
			value = std::stoi(str.substr(tempIndex, index - tempIndex));*/
			// after this function returns, index should be at the index after the value, which should be a comma: ,
		}
		//void _parseBoolean(const string& str, JsonValue& value, int& index) { // index is the first character of the boolean
		//	bool boolean = isTrue(str[index]);
		//	value = boolean;
		//	index += boolean ? 4 : 5;
		//}
		void parseString_impl(const string& str, JsonValue& value, int& index) { // index is the first character of the string // this is the parseString for values
			value = parseString__impl(str, index);
		}
		void parseArray_impl(const string& str, JsonValue& value, int& index) {
			value = JsonArray{};
			JsonArray& arr = value.get<JsonArray>();
			++index;
			while (1) {
				index = str.find_first_not_of("\n\t\r ,", index);
				if (index == string::npos) throw_impl(index);
				if (str[index] == ']') break;
				arr.push_back({});
				_parseValue(str, arr.back(), index);
			} ++index;
		}


		void parseInt_impl(const string& str, JsonValue& value, int& index) {
			value = int{};
			const char* sptr = str.data() + index;
			auto [ptr, ec] = std::from_chars(sptr, this->end, value.get<int>());
			if (ec == std::errc{})
				index += static_cast<int>(ptr - sptr);
			else throw_impl(index);
		}
		void parseFloat_impl(const string& str, JsonValue& value, int& index) {
			value = float{};
			const char* sptr = str.data() + index;
			auto [ptr, ec] = std::from_chars(sptr, this->end, value.get<float>());
			if (ec == std::errc{})
				index += static_cast<int>(ptr - sptr);
			else throw_impl(index);
		}
		/*class stringView {
		public:
			stringView(int b, int e) : begin(b), end(e) {
				if (b > e) throw std::logic_error("stringView: begin > end");
			}

			inline std::string get(const std::string& str) const {
				return std::string{ str.begin() + begin, str.begin() + end };
			}

			inline int size() const { return end - begin; }
			inline bool empty() const { return size() == 0; }
		private:
			int begin;
			int end;
		};*/
		bool isEscapedCharacter_impl(const string& str, int index) {
			--index;
			int counter = 0;
			while (index >= 0 && str[index] == '\\') {
				--index;
				// i intentionally didn't put a boundary check here, because if you put \\\\\\\\ for the entire json file, than you will kill my parser, and i will kill you.
				//if (index < 0) {
				//	// request by ChatGPT: "but to be precise, i highly suggest you to put a boundary check"
				//	while (1) {
				//		std::thread t([]() {
				//			while (1) {
				//				cout << "ERROR";
				//			}
				//			});
				//	}
				//}
				++counter;
			} return counter % 2;
		}
		string parseString__impl(const string& str, int& index) { // index is the first character of the string // this is the parseString for the fundamental process of find string between 2 " s
			++index;
			int tempIndex = index;
			do {
				index = str.find('"', index);
				if (index == string::npos) throw_impl(index);
			} while (isEscapedCharacter_impl(str, index++));

			//return { tempIndex, index++ };
			return str.substr(tempIndex, index - tempIndex - 1);
			// after this function returns, index should be at the index after ", which should be a comma: , or a colon: :
		}
		void throw_impl(int index) {
			string message = string{"[Error]: tx::JsonParser: at index: "} + std::to_string(index);
			throw std::runtime_error(message);
		}
	};


	JsonObject parseJson(const string& str) {
		static JsonParser parser;
		JsonObject root;
		parser.parse(str, root);
		return root;
	}




	// things to add:
	// operator<< for JosnValue / .str() function
	// comments
	// escaped character decodeing
	// better error messages
	//



}