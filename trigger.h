#ifndef _TRIGGER_H_
#define _TRIGGER_H_

#include <linux/limits.h>

struct trigger;

typedef void (*callback_t)(void*, void*);
typedef void (*loop_t)(void*);

struct trigger {
	char plugins_dir[PATH_MAX];
	struct plugin_manager *plug_mgr;
	loop_t loop;
};

void hook_callback(struct trigger *, callback_t, void *, void *);

void register_loop(struct trigger *, loop_t);

int run_trigger(struct trigger*, void*);

#endif
