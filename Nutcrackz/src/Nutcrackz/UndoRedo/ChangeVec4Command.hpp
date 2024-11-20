#pragma once

#include "Command.hpp"

#include "rtmcpp/Vector.hpp"

namespace Nutcrackz {

	class ChangeVec4Command : public Command
	{
	public:
		ChangeVec4Command(rtmcpp::Vec4& originalVec, rtmcpp::Vec4 newVec)
			: m_Vec(originalVec), m_NewVec(newVec), m_OldVec(0.0f, 0.0f, 0.0f, 1.0f)
		{
		}

		virtual void Execute() override
		{
			m_OldVec = m_Vec;
			m_Vec = m_NewVec;
		}

		virtual void Undo() override
		{
			m_Vec = m_OldVec;
		}

		virtual bool MergeWith(Command* other) override
		{
			ChangeVec4Command* commandToChange = dynamic_cast<ChangeVec4Command*>(other);

			if (commandToChange)
			{
				if (&commandToChange->m_Vec == &this->m_Vec)
				{
					commandToChange->m_NewVec = this->m_NewVec;
					return true;
				}
			}

			return false;
		}

	public:
		rtmcpp::Vec4& m_Vec;
		rtmcpp::Vec4 m_NewVec;
		rtmcpp::Vec4 m_OldVec;
	};

}