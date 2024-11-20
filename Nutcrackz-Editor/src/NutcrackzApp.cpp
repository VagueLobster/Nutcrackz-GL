#include <Nutcrackz.hpp>
#include <Nutcrackz/Core/EntryPoint.hpp>

#include "EditorLayer.hpp"

namespace Nutcrackz {

	class NutcrackzEditor : public Application
	{
	public:
		NutcrackzEditor(const ApplicationSpecification& specification, bool fullscreen, uint32_t width, uint32_t height)
			: Application(specification, true, width, height, fullscreen)
		{
			PushLayer(new EditorLayer());
		}

		~NutcrackzEditor()
		{
		}
	};

	Application* CreateApplication(ApplicationCommandLineArgs args)
	{
		ApplicationSpecification spec;
		spec.Name = "Nutcrackz Editor";                                            
		//spec.WorkingDirectory = "../Nutcrackz-Editor";
		spec.CommandLineArgs = args;
		spec.MaximizedOnLaunch = false;

		return new NutcrackzEditor(spec, false, 1600, 900);
	}
	 
}
