#include <stdio.h>
#include <shell.h>
#include <stdint.h>
#include <stdlib.h>

int main(void) {
	init_shell();

	while (1) {
		update_shell();
	}

	return 0;
}
