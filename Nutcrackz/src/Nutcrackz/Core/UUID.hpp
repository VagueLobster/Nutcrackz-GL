#pragma once

#include <xhash>

namespace Nutcrackz {

	class UUID
	{
	public:
		UUID();
		UUID(uint64_t uuid);
		UUID(const UUID&) = default;

		operator uint64_t() { return m_UUID; }
		operator uint64_t() const { return m_UUID; }
	private:
		uint64_t m_UUID;
	};

}

namespace std {

	//template <typename T> struct hash;

	template<>
	struct hash<Nutcrackz::UUID>
	{
		std::size_t operator()(const Nutcrackz::UUID& uuid) const
		{
			return uuid;
			//return (uint64_t)uuid;
		}
	};

}
