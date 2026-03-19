// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXJson
// File: json_parser.h

#pragma once
#include "impl/kvmap.h"
#include <charconv>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

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
using JsonArray = std::vector<JsonValue>;
using JsonMap = KVMap<std::string, JsonValue>;
using JsonMapHandle = KVMapHandle<std::string, JsonValue>;
using JsonPair = KVPair<std::string, JsonValue>;

class JsonObject {
	friend class JsonParser;

public:
	using iterator = JsonMap::iterator;
	using const_iterator = JsonMap::const_iterator;
	//JsonObject(JsonObject* in_parent = nullptr) : parent(in_parent) {}

	inline JsonValue& operator[](const std::string& key) { return this->members.at(key); }
	inline const JsonValue& operator[](const std::string& key) const { return this->members.at(key); }

	inline JsonPair& atIndex(int index) { return this->members.atIndex(index); }
	inline const JsonPair& atIndex(int index) const { return this->members.atIndex(index); }

	inline int size() const { return this->members.size(); }
	inline bool empty() const { return this->members.empty(); }

	inline bool exist(const std::string& key) const { return members.exist(key); }

	template <class T>
	inline const T& getOr(const std::string& key, const T& fallback) const;

	inline iterator begin() { return members.begin(); }
	inline const_iterator begin() const { return members.begin(); }
	inline iterator end() { return members.end(); }
	inline const_iterator end() const { return members.end(); }

private:
	JsonMap members;

	void validate() {
		members.validate();
		/*for (KVPair<std::string, JsonValue>& i : members) {
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
	    std::string,
	    JsonArray,
	    JsonObject>;
	friend class JsonParser;

public:
	JsonValue() {}
	JsonValue(const JT& in_var) : m_var(in_var) {}

	template <class T>
	inline bool is() const { return std::holds_alternative<T>(this->m_var); }
	inline JsonType type() const {
		return std::visit([](auto&& v) -> JsonType {
			using T = std::decay_t<decltype(v)>;
			if constexpr (std::is_same_v<T, bool>)
				return JsonType::Boolean;
			else if constexpr (std::is_same_v<T, int>)
				return JsonType::Int;
			else if constexpr (std::is_same_v<T, float>)
				return JsonType::Float;
			else if constexpr (std::is_same_v<T, std::string>)
				return JsonType::String;
			else if constexpr (std::is_same_v<T, JsonArray>)
				return JsonType::Array;
			else if constexpr (std::is_same_v<T, JsonObject>)
				return JsonType::JsonObject;
			else
				static_assert(sizeof(T) == 0, "Unhandled JsonValue type");
		},
		                  m_var);
	}
	template <class T>
	inline T& get() { return std::get<T>(this->m_var); }
	template <class T>
	inline const T& get() const { return std::get<T>(this->m_var); }

	inline explicit operator bool() const { return std::get<bool>(this->m_var); }

	inline JsonValue& operator[](int index) { return std::get<JsonArray>(this->m_var)[index]; }
	inline JsonValue& operator[](const std::string& key) { return std::get<JsonObject>(this->m_var)[key]; }
	inline const JsonValue& operator[](int index) const { return std::get<JsonArray>(this->m_var)[index]; }
	inline const JsonValue& operator[](const std::string& key) const { return std::get<JsonObject>(this->m_var)[key]; }

	inline JsonValue& operator=(bool val) {
		this->m_var = val;
		return *this;
	}
	inline JsonValue& operator=(int val) {
		this->m_var = val;
		return *this;
	}
	inline JsonValue& operator=(float val) {
		this->m_var = val;
		return *this;
	}
	inline JsonValue& operator=(const std::string& val) {
		this->m_var = val;
		return *this;
	}
	inline JsonValue& operator=(std::string&& val) {
		this->m_var = std::move(val);
		return *this;
	}
	inline JsonValue& operator=(const JsonArray& val) {
		this->m_var = val;
		return *this;
	}
	inline JsonValue& operator=(JsonArray&& val) {
		this->m_var = std::move(val);
		return *this;
	}
	inline JsonValue& operator=(const JsonObject& val) {
		this->m_var = val;
		return *this;
	}


private:
	JT m_var;
};

template <class T>
inline const T& JsonObject::getOr(const std::string& key, const T& fallback) const {
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
	void parse(const std::string& str, JsonObject& root);

private:
	const char* end = nullptr;

	void parseObject_impl(const std::string& str, JsonObject& root, int& index); // index is where the { of the object is in the entire std::string
	void parseKeyValue_impl(const std::string& str, JsonObject& root, int& index); // index is where the " of the key is in the entire string
	void parseValue_impl(const std::string& str, JsonValue& value, int& index); // index is the first character of the value
	void parseNumber_impl(const std::string& str, JsonValue& value, int& index); // index is the first character of the number
	void parseString_impl(const std::string& str, JsonValue& value, int& index); // index is the first character of the string // this is the parseString for values
	void parseArray_impl(const std::string& str, JsonValue& value, int& index);


	void parseInt_impl(const std::string& str, JsonValue& value, int& index);
	void parseFloat_impl(const std::string& str, JsonValue& value, int& index);
	bool isEscapedCharacter_impl(const std::string& str, int index);
	std::string parseString__impl(const std::string& str, int& index); // index is the first character of the string // this is the parseString for the fundamental process of find string between 2 " s

	void throw_impl(int index);
};


inline JsonObject parseJson(const std::string& str) {
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
} // namespace tx