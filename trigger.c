#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "plugin_manager.h"
#include "trigger.h"
#include "list.h"

void register_callback(struct trigger *trigger, callback_t callback, const char *name)
{
	register_module(trigger->plug_mgr, name);
	trigger->callback = callback;
}

static int run_hooks(struct list_head *head, int pre, void* args, void* data)
{
	struct module_hooks_node* curr_hook;
	struct list_head* current;
	int ret = 0;

	if (head) { 
		list_for_each(current, head) {
			curr_hook = list_entry(current, struct module_hooks_node, list);
			if (pre) {
				ret = curr_hook->m_hook->pre_hook(args, data);
			} else {
				ret = curr_hook->m_hook->post_hook(args, data);
			}
			if (ret) {
				//printf("plugin '%s' pre_hook return skip_later code\n",
				//		curr_hook->name);
				goto skip_later;	
			}
		}
	}

skip_later:
	return ret;
}

void hooked_callback(struct trigger *trigger, void* args, void* data)
{
	struct list_head* head = get_module_hooks(trigger->plug_mgr, "default");
 	int skip_later = 0;

	skip_later = run_hooks(head, 1, args, data);

	// registered callback
	if (!skip_later) {
		trigger->callback(args, data);
	}
	skip_later = 0;

	run_hooks(head, 0, args, data);
}

void register_loop(struct trigger *trigger, loop_t loop)
{
	trigger->loop = loop;
}

void hooked_loop(struct trigger *trigger, void *args)
{
	struct list_head* current;
	struct list_head* head = get_general_hooks(trigger->plug_mgr);
 	struct general_hooks_node* curr_hook;
	
	// for each init-hook
	list_for_each(current, head) {
		curr_hook = list_entry(current, struct general_hooks_node, list);
		curr_hook->g_hook->init_hook((void*)trigger);
	}

	trigger->loop(args);

	// for each exit-hook
	list_for_each(current, head) {
		curr_hook = list_entry(current, struct general_hooks_node, list);
		curr_hook->g_hook->exit_hook((void*)trigger);
	}
}

void init_trigger(struct trigger* trigger, const char *path)
{
	trigger->plug_mgr = init_plugin_manager(path);
}

void destory_trigger(struct trigger *trigger)
{
	destroy_plugin_manager(trigger->plug_mgr);
}

int run_trigger(struct trigger* trigger, void *args)
{
	int ret = -1;

	if(load_plugins(trigger->plug_mgr)) {
		fprintf(stderr, "failed loading plugins\n");
		goto plugin_error;
	}
	hooked_loop(trigger, args);

	destory_trigger(trigger);
	ret = 0;

plugin_error:
	return ret;
}