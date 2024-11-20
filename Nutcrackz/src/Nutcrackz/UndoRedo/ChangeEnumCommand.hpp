#pragma once

#include "Command.hpp"

namespace Nutcrackz {

	template<typename T>
	class ChangeEnumCommand final : public Command
	{
	public:
		ChangeEnumCommand(T& originalEnum, T newEnum)
			: m_Enum(originalEnum), m_NewEnum(newEnum), m_OldEnum(static_cast<T>(0))
		{
		}

		void Execute() override
		{
			m_OldEnum = m_Enum;
			m_Enum = m_NewEnum;
		}

		void Undo() override
		{
			m_Enum = m_OldEnum;
		}

		bool MergeWith(Command* other) override
		{
			ChangeEnumCommand<T>* commandToChange = dynamic_cast<ChangeEnumCommand<T>*>(other);

			if (commandToChange)
			{
				if (&commandToChange->m_Enum == &this->m_Enum)
				{
					commandToChange->m_NewEnum = this->m_NewEnum;
					return true;
				}
			}

			return false;
		}

	private:
		T& m_Enum;
		T m_NewEnum;
		T m_OldEnum;
	};

}