#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <linux/limits.h>

#include "trigger.h"

void my_callback(u_char	* args, int num)
{
	struct trigger *trigger = (struct trigger*)args;

	printf("The number is: %d. We started from: %d\n", num, trigger->initial_num);
}

int main(int argc, char **argv)
{
	struct trigger trigger;
	trigger.initial_num = 10;
	strncpy(trigger.args.plugins_dir, argv[1], PATH_MAX);
	register_callback(&trigger, my_callback);
	run_trigger(&trigger);
}