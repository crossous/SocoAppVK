#include "VulkanApp.h"
#include <iostream>

int main()
{
	Soco::TriangleApp app;

	try {
		app.Run();
	}
	catch (const std::runtime_error& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}