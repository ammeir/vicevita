
#include "view.h"
#include "controller.h"

extern "C" {
#include "main.h"
#include "machine.h"
}


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

