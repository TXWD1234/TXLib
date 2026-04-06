// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXJson
// File: json_parser.hpp

#pragma once
#include "impl/kvmap.hpp"
#include <charconv>
#include <stdexcept>

#include "json_types.hpp"

namespace tx {

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