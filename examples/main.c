#include <stdio.h>
#include <unistd.h>
#include <signal.h>
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
typedef int (*bb_callback_t)(void*, int);

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

int callback(void* priv, int current)
{
	struct callback_data cdata = {.current = current};
	struct callback_args *cargs = (struct callback_args*)priv;

	return handle_callback(cargs->trigger, "default", priv, (void*)&cdata);
}

int my_loop(void* args)
{
	struct callback_args *cargs = (struct callback_args*)args;

	black_box_loop(cargs->initial, callback, args);
	return 0;
}

int my_callback(void* args, void* data)
{
	struct callback_args *cargs = (struct callback_args*)args;
	struct callback_data *cdata = (struct callback_data*)data;

	printf("The number is: %d. We started from: %d\n", cdata->current, cargs->initial);
	return 0;
}

void clean_exit(int sig)
{
	exit(0);
}

int main(int argc, char **argv)
{
	int ret = -1;
	struct trigger trigger;
	struct callback_args cargs = {.initial = 10, .trigger = &trigger};

	if (init_trigger(&trigger, argc > 1 ? argv[1] : "")) {
		goto error_init;
	}
	if (register_callback(&trigger, "default", my_callback)) {
		goto error_register_default;
	}
	register_loop(&trigger, my_loop);
	signal(SIGINT, clean_exit);
	ret = run_trigger(&trigger, (void*)&cargs);

error_register_default:
	destory_trigger(&trigger);
error_init:
	return ret;
}