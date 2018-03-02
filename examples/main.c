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
void black_box_loop(int initial, bb_callback_t cb_each, bb_callback_t cb_triplet, void* priv)
{
	int i = initial;

	while (true) {
		cb_each(priv, i);
		if ((i - initial) % 3 == 0) {
			cb_triplet(priv, i);
		}
		i++;
		sleep(3);
	}
}

void each_callback(void* args, void* data)
{
	struct callback_args *cargs = (struct callback_args*)args;
	struct callback_data *cdata = (struct callback_data*)data;

	printf("The number is: %d. We started from: %d\n", cdata->current, cargs->initial);
}

void triplets_callback(void* args, void* data)
{
	struct callback_args *cargs = (struct callback_args*)args;
	struct callback_data *cdata = (struct callback_data*)data;

	printf("This is triplet number: %d\n", (cdata->current - cargs->initial) / 3);
}

void callback(void* priv, int current)
{
	struct callback_data cdata = {.current = current};
	struct callback_args *cargs = (struct callback_args*)priv;

	hook_callback(cargs->trigger, "", priv, (void*)&cdata);
}

void my_loop(void* args)
{
	struct callback_args *cargs = (struct callback_args*)args;

	black_box_loop(cargs->initial, callback, args);
}

int main(int argc, char **argv)
{
	struct trigger trigger;
	struct callback_args cargs = {.initial = 10, .trigger = &trigger};

	if (argc > 1){
		strncpy(trigger.plugins_dir, argv[1], PATH_MAX);
	}
	register_callback("each", each_callback);
	register_callback("triplets", triplets_callback);
	register_loop(&trigger, my_loop);
	run_trigger(&trigger, (void*)&cargs);
	return 0;
}
