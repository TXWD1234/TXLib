// Copyright@TXLib All rights reserved.
// Author: TX Studio: TX_Jerry
// Module: TXJson
// File: json_parser.cpp

#include "impl/json_parser.h"

namespace tx {

void JsonParser::parse(const std::string& str, JsonObject& root) {
	int index = 0;
	this->end = str.data() + str.size();
	parseObject_impl(str, root, index);
}

void JsonParser::parseObject_impl(const std::string& str, JsonObject& root, int& index) { // index is where the { of the object is in the entire std::string
	++index;
	KVMap<std::string, JsonValue>& rootmap = root.members;

	while (1) {
		index = str.find_first_not_of("\n\t\r ,", index);
		if (index == std::string::npos) throw_impl(index);
		if (str[index] == '}') break;
		if (str[index] != '"') throw_impl(index);
		parseKeyValue_impl(str, root, index);
	}

	rootmap.validate();
	++index;
}
void JsonParser::parseKeyValue_impl(const std::string& str, JsonObject& root, int& index) { // index is where the " of the key is in the entire string
	JsonMap& rootmap = root.members;
	// determind key
	JsonMapHandle hValue = rootmap.insertMulti(parseString__impl(str, index));

	index = str.find_first_not_of(": ", index);
	if (index == std::string::npos) throw_impl(index);
	parseValue_impl(str, hValue.get(), index);
}
void JsonParser::parseValue_impl(const std::string& str, JsonValue& value, int& index) { // index is the first character of the value
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
		} else if (str.compare(index, 4, "true") == 0) {
			value = true;
			index += 4;
		} else if (str.compare(index, 5, "false") == 0) {
			value = false;
			index += 5;
		} else {
			throw_impl(index);
		}
	}
	// after this function returns, index should be at the index after the value, which should be a comma: ,
}
void JsonParser::parseNumber_impl(const std::string& str, JsonValue& value, int& index) { // index is the first character of the number
	// determind is int or float
	int dotIndex = str.find('.', index),
	    endIndex = str.find_first_of("}, ]", index);
	if (dotIndex == std::string::npos || dotIndex > endIndex) {
		parseInt_impl(str, value, index);
	} else {
		parseFloat_impl(str, value, index);
	}
	/*int tempIndex = index;
			while (isNumber(str[index])) ++index;
			value = std::stoi(str.substr(tempIndex, index - tempIndex));*/
	// after this function returns, index should be at the index after the value, which should be a comma: ,
}
//void _parseBoolean(const std::string& str, JsonValue& value, int& index) { // index is the first character of the boolean
//	bool boolean = isTrue(str[index]);
//	value = boolean;
//	index += boolean ? 4 : 5;
//}
void JsonParser::parseString_impl(const std::string& str, JsonValue& value, int& index) { // index is the first character of the string // this is the parseString for values
	value = parseString__impl(str, index);
}
void JsonParser::parseArray_impl(const std::string& str, JsonValue& value, int& index) {
	value = JsonArray{};
	JsonArray& arr = value.get<JsonArray>();
	++index;
	while (1) {
		index = str.find_first_not_of("\n\t\r ,", index);
		if (index == std::string::npos) throw_impl(index);
		if (str[index] == ']') break;
		arr.push_back({});
		parseValue_impl(str, arr.back(), index);
	}
	++index;
}


void JsonParser::parseInt_impl(const std::string& str, JsonValue& value, int& index) {
	value = int{};
	const char* sptr = str.data() + index;
	auto [ptr, ec] = std::from_chars(sptr, this->end, value.get<int>());
	if (ec == std::errc{})
		index += static_cast<int>(ptr - sptr);
	else
		throw_impl(index);
}
void JsonParser::parseFloat_impl(const std::string& str, JsonValue& value, int& index) {
	value = float{};
	const char* sptr = str.data() + index;
	auto [ptr, ec] = std::from_chars(sptr, this->end, value.get<float>());
	if (ec == std::errc{})
		index += static_cast<int>(ptr - sptr);
	else
		throw_impl(index);
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
bool JsonParser::isEscapedCharacter_impl(const std::string& str, int index) {
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
	}
	return counter % 2;
}
std::string JsonParser::parseString__impl(const std::string& str, int& index) { // index is the first character of the string // this is the parseString for the fundamental process of find string between 2 " s
	++index;
	int tempIndex = index;
	do {
		index = str.find('"', index);
		if (index == std::string::npos) throw_impl(index);
	} while (isEscapedCharacter_impl(str, index++));

	//return { tempIndex, index++ };
	return str.substr(tempIndex, index - tempIndex - 1);
	// after this function returns, index should be at the index after ", which should be a comma: , or a colon: :
}
void JsonParser::throw_impl(int index) {
	std::string message = std::string{ "[Error]: tx::JsonParser: at index: " } + std::to_string(index);
	throw std::runtime_error(message);
}
} // namespace tx