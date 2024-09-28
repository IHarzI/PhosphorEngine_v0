#pragma once

#include <optional>
#include <utility>
template <class T>
struct PH_Optional
{
	PH_Optional(T Value) { OptionalValue = Value; };
	PH_Optional() {};
	bool IsSet() const { return OptionalValue.has_value(); };
	void SetValue(T Value) { OptionalValue = Value; };
	const T& GetValue() const { return OptionalValue.value(); };

	template<typename ...Args>
	void Emplace(Args&&... args) { OptionalValue.emplace(std::forward<Args>(args)...); };
private:
	std::optional<T> OptionalValue;
};