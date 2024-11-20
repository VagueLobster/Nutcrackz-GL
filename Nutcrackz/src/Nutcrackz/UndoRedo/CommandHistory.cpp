#include "nzpch.hpp"

#include "CommandHistory.hpp"

namespace Nutcrackz {

	std::array<Command*, 1000> CommandHistory::m_Commands = {};
	uint32_t CommandHistory::m_CommandSize = 0;
	uint32_t CommandHistory::m_CommandIndex = 0;

	// Example use: CommandHistory::AddCommand(new ChangeVec3Command(player.Translation, new rtmcpp::Vec3(1.0f, 1.0f, 1.0f)));

	void CommandHistory::AddCommand(Command* cmd)
	{
		cmd->Execute();

		//if (m_CommandIndex < m_CommandSize - 1 && m_CommandSize > 0)
		//{
		//	for (uint32_t i = m_CommandSize - 1; i > m_CommandIndex; i--)
		//		delete m_Commands[i];
		//}

		if (m_CommandSize < m_Commands.size() - 1)
		{
			if (m_CommandIndex < m_CommandSize - 1)
				m_CommandSize = m_CommandIndex + 1;

			m_Commands[m_CommandSize] = cmd;

			m_CommandSize++;

			if (m_CommandSize > 1 && m_Commands[m_CommandSize - 1] != nullptr && m_Commands[m_CommandSize - 2] != nullptr && m_Commands[m_CommandSize - 1]->CanMerge() && m_Commands[m_CommandSize - 2]->CanMerge())
			{
				if (m_Commands[m_CommandSize - 1]->MergeWith(m_Commands[m_CommandSize - 2]))
				{
					delete m_Commands[m_CommandSize - 1];
					m_CommandSize--;
				}
			}
		}

		m_CommandIndex = m_CommandSize - 1;
	}

	void CommandHistory::RemoveCommand()
	{
		if (m_CommandIndex < m_CommandSize - 1 && m_CommandSize > 1)
		{
			delete m_Commands[0];
			m_CommandSize--;
		}
	}

	void CommandHistory::SetNoMergeMostRecent()
	{
		if (m_CommandSize - 1 >= 0 && m_CommandSize < m_Commands.size() - 1)
		{
			m_Commands[m_CommandSize - 1]->SetNoMerge();
		}
	}

	void CommandHistory::Undo()
	{
		if (m_CommandIndex > 0 && m_Commands[m_CommandIndex] != nullptr)
		{
			m_Commands[m_CommandIndex]->Undo();
			m_CommandIndex--;
		}
	}

	void CommandHistory::Redo()
	{
		uint32_t redoCommand = m_CommandIndex + 1;
		if (redoCommand < m_CommandSize && redoCommand >= 0 && m_Commands[redoCommand] != nullptr)
		{
			m_Commands[redoCommand]->Execute();
			m_CommandIndex++;
		}
	}

	void CommandHistory::AddAndRemoveCommand(Command* cmd)
	{
		if (m_CommandSize >= m_Commands.size() - 1)
			RemoveCommand();

		AddCommand(cmd);
		SetNoMergeMostRecent();
	}

	void CommandHistory::ResetCommandsHistory()
	{
		if (m_CommandIndex < m_CommandSize - 1 && m_CommandSize > 0)
		{
			for (uint32_t i = m_CommandSize - 1; i > 0; i--)
				delete m_Commands[i];
		}

		m_CommandSize = 0;
		m_CommandIndex = 0;
	}

}