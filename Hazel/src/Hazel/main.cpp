#include "hzpch.h"
#include "Hazel/Core/Application.h"
int main(int argc, char** argv)
{
	GameEngine::ApplicationSpecification spec;
	spec.Name = "Hazelnut";
	spec.CommandLineArgs = { argc, argv };
	GameEngine::Application* app = new GameEngine::Application(spec);
	app->Tick();
	delete app;
	
}
