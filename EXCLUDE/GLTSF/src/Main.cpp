#include "App.hpp"
#include <stdexcept>

int main(int argc, char *argv[])
{
	try
	{
		App theApp;
		theApp.Run();
	}
	catch (const std::exception& e)
	{
		printf("Error: %s\n", e.what());
		return 1;
	}
	return 0;
}
