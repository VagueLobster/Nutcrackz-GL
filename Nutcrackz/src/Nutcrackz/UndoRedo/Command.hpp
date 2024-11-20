#pragma once

#include <stdint.h>

namespace Nutcrackz {

	class Command
	{
	public:
		virtual ~Command() {}
		virtual void Execute() = 0;
		virtual void Undo() = 0;
		virtual bool MergeWith(Command* other) = 0;

		void SetNoMerge() { m_CanMerge = false; }
		bool CanMerge() const { return m_CanMerge; }

	private:
		bool m_CanMerge = true;

		//static int64_t m_ID;
	};

	/*template<size_t C>
	class CommandQueue {
	public:

		void Push(Command c)
		{
			stack.Push(c);
		}

		std::optional<Command> Pop()
		{
			return stack.Pop();
		}

		void Undo()
		{
			stack.Back().Undo();
			stack.Pop();
		}

	private:

		Stack<Command, C> stack;
	};*/

}