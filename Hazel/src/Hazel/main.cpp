#include "hzpch.h"
#include "Hazel/Core/Application.h"
#include "LearnCpp/learn.h"
int main(int argc, char** argv)
{
	LearnClass learn;
	if (learn.isLearning) {
		learn.LearnEntryPoit();
		return 0;
	}
	GameEngine::ApplicationSpecification spec;
	spec.Name = "Hazelnut";
	spec.CommandLineArgs = { argc, argv };
	GameEngine::Application* app = new GameEngine::Application(spec);
	app->Tick();
	delete app;
	
}


// CPPѧϰ
