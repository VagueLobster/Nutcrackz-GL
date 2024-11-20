#pragma once

#include "Nutcrackz/Containers/Stack.hpp"
#include "Command.hpp"

#include <stack>

namespace Nutcrackz {

	class CommandHistory
	{
	public:
		static void AddCommand(Command* cmd);
		static void RemoveCommand();
		static void Undo();
		static void Redo();

		static void AddAndRemoveCommand(Command* cmd);
		static void ResetCommandsHistory();

		static uint32_t GetCommandSize() { return m_CommandSize; }
		static bool CanUndo() { return m_CommandIndex > 0; }
		static bool CanRedo() { uint32_t redoCmd = m_CommandIndex + 1; return (redoCmd < m_CommandSize) && (redoCmd > 0); }

	private:
		static void SetNoMergeMostRecent();

	private:
		static std::array<Command*, 1000> m_Commands;
		//static Stack<Command*, 1000> m_Commands;
		static uint32_t m_CommandSize;
		static uint32_t m_CommandIndex;
	};

}