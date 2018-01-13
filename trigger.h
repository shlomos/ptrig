#ifndef _TRIGGER_H_
#define _TRIGGER_H_

#include <linux/limits.h>

struct trigger;

typedef void (*callback_t)(void*, void*);
typedef void (*loop_t)(struct trigger *, void*);

struct trigger_args {
	char plugins_dir[PATH_MAX];
};

struct trigger {
	struct trigger_args args;
	int initial_num;
	struct plugin_manager *plug_mgr;
	callback_t callback;
	loop_t loop;
};

int loop();

void hooked_callback(struct trigger*, void*, void*);

void register_callback(struct trigger*, callback_t);

void register_loop(struct trigger *, loop_t);

int run_trigger(struct trigger*, void*);

#endif