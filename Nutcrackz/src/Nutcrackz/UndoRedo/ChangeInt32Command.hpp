#pragma once

#include "Command.hpp"

namespace Nutcrackz {

	class ChangeInt32Command : public Command
	{
	public:
		ChangeInt32Command(int32_t& originalInt, int32_t newInt)
			: m_Int(originalInt), m_NewInt(newInt), m_OldInt(0)
		{
		}

		void Execute() override
		{
			m_OldInt = m_Int;
			m_Int = m_NewInt;
		}

		void Undo() override
		{
			m_Int = m_OldInt;
		}

		bool MergeWith(Command* other) override
		{
			ChangeInt32Command* commandToChange = dynamic_cast<ChangeInt32Command*>(other);

			if (commandToChange)
			{
				if (&commandToChange->m_Int == &this->m_Int)
				{
					commandToChange->m_NewInt = this->m_NewInt;
					return true;
				}
			}

			return false;
		}

	private:
		int32_t& m_Int;
		int32_t m_NewInt;
		int32_t m_OldInt;
	};

}