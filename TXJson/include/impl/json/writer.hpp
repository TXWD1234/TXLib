// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXJson

#pragma once
#include "impl/basic_utils.hpp"
#include "impl/json/types.hpp"

namespace tx {
/**
 * This Writer does not format the json. so everything will be in one line (Minified)
 */
class JsonWriter {
public:
	static void write(const JsonObject& root, std::ostream& out) {
		writeObject(out, root);
	}

	JsonWriter() = delete;

private:
	static void writeObject(std::ostream& out, const JsonObject& object) {
		out << '{';
		bool isFirst = true;
		for (const auto& i : object) {
			if (!isFirst)
				out << ',';
			else
				isFirst = false;

			writeKey(out, i.k());
			writeValue(out, i.v());
		}
		out << '}';
	}

	static void writeValue(std::ostream& out, const JsonValue& value) {
		switch (value.type()) {
		case JsonType::Boolean:
			writeBoolean(out, value.get<bool>());
			break;
		case JsonType::Int:
			writeInt(out, value.get<int>());
			break;
		case JsonType::Float:
			writeFloat(out, value.get<float>());
			break;
		case JsonType::String:
			writeString(out, value.get<std::string>());
			break;
		case JsonType::Array:
			writeArray(out, value.get<JsonArray>());
			break;
		case JsonType::JsonObject:
			writeObject(out, value.get<JsonObject>());
			break;
		}
	}
	static void writeBoolean(std::ostream& out, bool value) {
		out << (value ? "true" : "false");
	}
	static void writeInt(std::ostream& out, int value) {
		out << value;
	}
	static void writeFloat(std::ostream& out, float value) {
		char buffer[64];
		auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);

		if (ec == std::errc()) {
			out.write(buffer, ptr - buffer);

			bool hasDecimalOrExp = std::any_of(buffer, ptr, [](char c) {
				return c == '.' || c == 'e';
			});

			if (!hasDecimalOrExp) {
				out << ".0";
			}
		}
	}
	static void writeString(std::ostream& out, std::string_view value) {
		out << "\"" << value << "\"";
	}
	static void writeArray(std::ostream& out, const JsonArray& value) {
		out << '[';
		bool isFirst = true;
		for (const JsonValue& i : value) {
			if (!isFirst)
				out << ',';
			else
				isFirst = false;
			writeValue(out, i);
		}
		out << ']';
	}
	static void writeKey(std::ostream& out, std::string_view value) {
		out << "\"" << value << "\":";
	}
};

inline void JsonObject::write(std::ostream& out) {
	JsonWriter::write(*this, out);
}
} // namespace tx