#include <stdio.h>
#include <shell.h>
#include <stdint.h>
#include <stdlib.h>

int main(void) {
	init_shell();

	while (update_shell());

	return 0;
}
