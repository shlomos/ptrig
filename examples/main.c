#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <linux/limits.h>

#include "trigger.h"

struct callback_args {
	struct trigger *trigger;
	int initial;
};

struct callback_data {
	int current;
};

/*
* An example black-box callback
*/
typedef void (*bb_callback_t)(void*, int);

/*
* An example loop you cannot change, must use its API
*/
void black_box_loop(int initial, bb_callback_t cb, void* priv)
{
	int i = initial;

	while (true) {
		cb(priv, i);
		i++;
		sleep(5);
	}
}

void callback(void* priv, int current)
{
	struct callback_data cdata = {.current = current};
	struct callback_args *cargs = (struct callback_args*)priv;

	hooked_callback(cargs->trigger, priv, (void*)&cdata);
}

void my_loop(struct trigger* trigger, void* args)
{
	struct callback_args *cargs = (struct callback_args*)args;

	black_box_loop(cargs->initial, callback, (void*)&cargs);
}

void my_callback(void* args, void* data)
{
	struct callback_args *cargs = (struct callback_args*)args;
	struct callback_data *cdata = (struct callback_data*)data;

	printf("The number is: %d. We started from: %d\n", cdata->current, cargs->initial);
}

int main(int argc, char **argv)
{
	struct trigger trigger;
	struct callback_args cargs = {.initial = 10, .trigger = &trigger};

	strncpy(trigger.args.plugins_dir, argv[1], PATH_MAX);
	register_callback(&trigger, my_callback);
	register_loop(&trigger, my_loop);
	run_trigger(&trigger, (void*)&cargs);
	return 0;
}