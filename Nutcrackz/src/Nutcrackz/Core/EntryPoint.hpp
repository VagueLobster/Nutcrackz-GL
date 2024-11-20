#pragma once

#include "Nutcrackz/Core/Core.hpp"
#include "Nutcrackz/Core/Application.hpp"
#include "Nutcrackz/Core/ConsoleLog.hpp"

#ifdef NZ_PLATFORM_WINDOWS

extern Nutcrackz::Application *Nutcrackz::CreateApplication(ApplicationCommandLineArgs args);

namespace Nutcrackz {

	int Main(int argc, char** argv)
	{
		//NZ_PROFILE_BEGIN_SESSION("Nutcrackz", "NutcrackzLeaksProfiling.json");
		Nutcrackz::Log::Init();
		Nutcrackz::ConsoleLog::Init();

		Nutcrackz::Application* app = Nutcrackz::CreateApplication({ argc, argv });
		NZ_CORE_ASSERT(app, "Client Application is null!");
		app->Run();
		delete app;
		//NZ_PROFILE_END_SESSION();

		return 0;
	}

}

#ifdef NZ_DIST

#include <Windows.h>

int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, int cmdshow)
{
	return Nutcrackz::Main(__argc, __argv);
}

#else

int main(int argc, char** argv)
{
	return Nutcrackz::Main(argc, argv);
}

#endif

#endif
