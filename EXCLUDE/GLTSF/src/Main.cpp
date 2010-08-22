#include "App.hpp"
#include <stdexcept>

int main(int argc, char *argv[])
{
	int Result = EXIT_SUCCESS;
	try
	{
		App theApp;
		theApp.Run();
	}
	catch (const std::exception& e)
	{
		printf("Error: %s\n", e.what());
		Result = EXIT_FAILURE;
	}
	catch (...)
	{
		printf("Unhandled exception\n");
		Result = EXIT_FAILURE;
	}
	system("PAUSE");
	return Result;
}
