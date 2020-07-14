
#include "view.h"
#include "controller.h"

extern "C" {
#include "main.h"
#include "machine.h"
}

// Increase heap size to 64MB (default 32MB) to prevent memory allocation failures. 
// Loading a crt game alone allocates 16MB from the heap so malloc can easily fail.
int _newlib_heap_size_user = 64 * 1024 * 1024;

int main(int argc, char *argv[])
{
	View		view;
	Controller	controller; 

	controller.init(&view);
	view.init(&controller);

	main_program(argc, argv);

	return 0;
}

void main_exit()
{
	// This function will be called at program exit
    machine_shutdown();
}

