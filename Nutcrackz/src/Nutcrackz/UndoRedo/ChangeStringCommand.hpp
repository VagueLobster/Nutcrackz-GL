#pragma once

#include "Command.hpp"

#include <string>

namespace Nutcrackz {

	class ChangeStringCommand : public Command
	{
	public:
		ChangeStringCommand(std::string& originalString, std::string newString)
			: m_String(originalString), m_NewString(newString), m_OldString("")
		{
		}

		void Execute() override
		{
			m_OldString = m_String;
			m_String = m_NewString;
		}

		void Undo() override
		{
			m_String = m_OldString;
		}

		bool MergeWith(Command* other) override
		{
			ChangeStringCommand* commandToChange = dynamic_cast<ChangeStringCommand*>(other);

			if (commandToChange)
			{
				if (&commandToChange->m_String == &this->m_String)
				{
					commandToChange->m_NewString = this->m_NewString;
					return true;
				}
			}

			return false;
		}

	private:
		std::string& m_String;
		std::string m_NewString;
		std::string m_OldString;
	};

}