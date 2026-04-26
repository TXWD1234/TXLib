// Copyright (c) 2025 TXLib. Licensed under the MIT License.
// Module: TXJson

#pragma once
#include "impl/kvmap.hpp"
#include <concepts>
#include <string>
#include <variant>
#include <vector>
#include <string_view>

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

using JsonMergeReplace = Map::Replace;
using JsonMergeIgnore = Map::Ignore;

template <class Func>
concept JsonConflictResolveFunc = Map::ConflictResolveFunc<Func, std::string, JsonValue>;

class JsonObject {
	friend class JsonParser;

public:
	using iterator = JsonMap::iterator;
	using const_iterator = JsonMap::const_iterator;
	//JsonObject(JsonObject* in_parent = nullptr) : parent(in_parent) {}

	inline JsonValue& operator[](std::string_view key) { return this->members.at(key); }
	inline const JsonValue& operator[](std::string_view key) const { return this->members.at(key); }

	inline JsonValue& get(std::string_view key) { return this->members.at(key); }
	inline const JsonValue& get(std::string_view key) const { return this->members.at(key); }

	inline JsonPair& atIndex(int index) { return this->members.atIndex(index); }
	inline const JsonPair& atIndex(int index) const { return this->members.atIndex(index); }

	inline int size() const { return this->members.size(); }
	inline bool empty() const { return this->members.empty(); }

	inline bool exist(std::string_view key) const { return members.exist(key); }

	template <class T>
	inline T getOr(std::string_view key, const T& fallback) const;
	template <class T>
	inline T* get(std::string_view key);
	template <class T>
	inline const T* get(std::string_view key) const;

	inline iterator find(std::string_view key) { return members.find(key); }
	inline const_iterator find(std::string_view key) const { return members.find(key); }

	inline iterator begin() { return members.begin(); }
	inline const_iterator begin() const { return members.begin(); }
	inline iterator end() { return members.end(); }
	inline const_iterator end() const { return members.end(); }

	// utilities

	template <JsonConflictResolveFunc Func = JsonMergeReplace>
	inline void merge(const JsonObject& other, Func&& resolve = JsonMergeReplace{}) {
		members.merge(other.members, std::forward<Func>(resolve));
	}
	template <JsonConflictResolveFunc Func = JsonMergeReplace>
	inline void merge(JsonObject&& other, Func&& resolve = JsonMergeReplace{}) {
		members.merge(std::move(other.members), std::forward<Func>(resolve));
	}

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
	inline JsonValue& operator[](std::string_view key) { return std::get<JsonObject>(this->m_var)[key]; }
	inline const JsonValue& operator[](int index) const { return std::get<JsonArray>(this->m_var)[index]; }
	inline const JsonValue& operator[](std::string_view key) const { return std::get<JsonObject>(this->m_var)[key]; }

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
inline T* JsonObject::get(std::string_view key) {
	if (!exist(key)) return nullptr;
	JsonValue& val = get(key);
	if (val.is<T>()) return &val.get<T>();
	return nullptr;
}
template <class T>
inline const T* JsonObject::get(std::string_view key) const {
	if (!exist(key)) return nullptr;
	const JsonValue& val = get(key);
	if (val.is<T>()) return &val.get<T>();
	return nullptr;
}
template <class T>
inline T JsonObject::getOr(std::string_view key, const T& fallback) const {
	const auto* valptr = get<T>(key);
	if (valptr) return *valptr;
	return fallback;
}
} // namespace tx
