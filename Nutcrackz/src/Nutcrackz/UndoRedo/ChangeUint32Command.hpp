#pragma once

#include "Command.hpp"

namespace Nutcrackz {

	class ChangeUint32Command : public Command
	{
	public:
		ChangeUint32Command(uint32_t& originalUint, uint32_t newUint)
			: m_Uint(originalUint), m_NewUint(newUint), m_OldUint(0)
		{
		}

		void Execute() override
		{
			m_OldUint = m_Uint;
			m_Uint = m_NewUint;
		}

		void Undo() override
		{
			m_Uint = m_OldUint;
		}

		bool MergeWith(Command* other) override
		{
			ChangeUint32Command* commandToChange = dynamic_cast<ChangeUint32Command*>(other);

			if (commandToChange)
			{
				if (&commandToChange->m_Uint == &this->m_Uint)
				{
					commandToChange->m_NewUint = this->m_NewUint;
					return true;
				}
			}

			return false;
		}

	private:
		uint32_t& m_Uint;
		uint32_t m_NewUint;
		uint32_t m_OldUint;
	};

}