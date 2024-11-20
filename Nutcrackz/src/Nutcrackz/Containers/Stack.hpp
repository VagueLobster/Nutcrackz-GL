#pragma once

#include <deque>
#include <optional>

namespace Nutcrackz {

	template<typename T, size_t C>
	class Stack {
	public:
		void Push(T t)
		{
			if (data.size() >= C) {
				data.pop_front();
			}
			data.push_back(t);
		}

		std::optional<T> Pop()
		{
			if (data.empty()) {
				return std::nullopt;
			}
			auto t = data.back();
			data.pop_back();
			return t;
		}

		T& Back() const
		{
			return data.back();
		}

		std::deque<T> data;
	};

}