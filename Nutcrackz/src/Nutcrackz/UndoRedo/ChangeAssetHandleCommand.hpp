#pragma once

#include "Nutcrackz/Asset/Asset.hpp"

#include "Command.hpp"

namespace Nutcrackz {

	class ChangeAssetHandleCommand : public Command
	{
	public:
		ChangeAssetHandleCommand(AssetHandle& originalHandle, AssetHandle newHandle)
			: m_Handle(originalHandle), m_NewHandle(newHandle), m_OldHandle(0)
		{
		}

		void Execute() override
		{
			m_OldHandle = m_Handle;
			m_Handle = m_NewHandle;
		}

		void Undo() override
		{
			m_Handle = m_OldHandle;
		}

		bool MergeWith(Command* other) override
		{
			ChangeAssetHandleCommand* commandToChange = dynamic_cast<ChangeAssetHandleCommand*>(other);

			if (commandToChange)
			{
				if (&commandToChange->m_Handle == &this->m_Handle)
				{
					commandToChange->m_NewHandle = this->m_NewHandle;
					return true;
				}
			}

			return false;
		}

	private:
		AssetHandle& m_Handle;
		AssetHandle m_NewHandle;
		AssetHandle m_OldHandle;
	};

}