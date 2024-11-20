#pragma once

#include "Command.hpp"

#include "rtmcpp/Vector.hpp"

namespace Nutcrackz {

	class ChangeVec3Command : public Command
	{
	public:
		ChangeVec3Command(rtmcpp::Vec3& originalVec, rtmcpp::Vec3& newVec)
		: m_Vec(originalVec), m_NewVec(newVec), m_OldVec(0.0f, 0.0f, 0.0f)
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
			ChangeVec3Command* commandToChange = dynamic_cast<ChangeVec3Command*>(other);

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

	private:
		rtmcpp::Vec3& m_Vec;
		rtmcpp::Vec3 m_NewVec;
		rtmcpp::Vec3 m_OldVec;
	};

}