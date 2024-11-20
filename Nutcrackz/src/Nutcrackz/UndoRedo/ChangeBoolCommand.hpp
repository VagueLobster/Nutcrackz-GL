#pragma once

#include "Command.hpp"

namespace Nutcrackz {

	class ChangeBoolCommand : public Command
	{
	public:
		ChangeBoolCommand(bool& originalBool, bool newBool)
			: m_Bool(originalBool), m_NewBool(newBool), m_OldBool(false)
		{
		}

		void Execute() override
		{
			m_OldBool = m_Bool;
			m_Bool = m_NewBool;
		}

		void Undo() override
		{
			m_Bool = m_OldBool;
		}

		bool MergeWith(Command* other) override
		{
			ChangeBoolCommand* commandToChange = dynamic_cast<ChangeBoolCommand*>(other);

			if (commandToChange)
			{
				if (&commandToChange->m_Bool == &this->m_Bool)
				{
					commandToChange->m_NewBool = this->m_NewBool;
					return true;
				}
			}

			return false;
		}

	private:
		bool& m_Bool;
		bool m_NewBool;
		bool m_OldBool;
	};

}