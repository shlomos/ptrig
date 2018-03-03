#ifndef _TRIGGER_H_
#define _TRIGGER_H_

#include <linux/limits.h>

struct trigger;

typedef void (*callback_t)(void*, void*);
typedef void (*loop_t)(void*);

struct trigger {
	struct plugin_manager *plug_mgr;
	callback_t callback;
	loop_t loop;
};

void hooked_callback(struct trigger*, void*, void*);

void register_callback(struct trigger*, callback_t, const char*);

void register_loop(struct trigger *, loop_t);

void init_trigger(struct trigger *, const char *path);

void destory_trigger(struct trigger *);

int run_trigger(struct trigger*, void*);

#endif