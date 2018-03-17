#ifndef _TRIGGER_H_
#define _TRIGGER_H_

#include <linux/limits.h>

struct trigger;

typedef int (*callback_t)(void *, void *);
typedef int (*loop_t)(void *);

struct trigger {
	struct plugin_manager *plug_mgr;
	struct callbacks_table *callbacks;
	loop_t loop;
};

int register_callback(struct trigger *trigger, const char *name, callback_t cb);

void register_loop(struct trigger *trigger, loop_t loop);

int init_trigger(struct trigger *trigger, const char *path);

int handle_callback(struct trigger *trigger, const char *name,
		void *args, void *data);

void destory_trigger(struct trigger *trigger);

int run_trigger(struct trigger *trigger, void *args);

#endif
