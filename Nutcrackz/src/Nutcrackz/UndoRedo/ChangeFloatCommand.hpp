#pragma once

#include "Command.hpp"

namespace Nutcrackz {

	class ChangeFloatCommand : public Command
	{
	public:
		ChangeFloatCommand(float& originalFloat, float newFloat)
			: m_Float(originalFloat), m_NewFloat(newFloat), m_OldFloat(0.0f)
		{
		}

		void Execute() override
		{
			m_OldFloat = m_Float;
			m_Float = m_NewFloat;
		}

		void Undo() override
		{
			m_Float = m_OldFloat;
		}

		bool MergeWith(Command* other) override
		{
			ChangeFloatCommand* commandToChange = dynamic_cast<ChangeFloatCommand*>(other);

			if (commandToChange)
			{
				if (&commandToChange->m_Float == &this->m_Float)
				{
					commandToChange->m_NewFloat = this->m_NewFloat;
					return true;
				}
			}

			return false;
		}

	private:
		float& m_Float;
		float m_NewFloat;
		float m_OldFloat;
	};

}