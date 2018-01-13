#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>

#include "plugin_manager.h"
#include "trigger.h"
#include "list.h"

void register_callback(struct trigger *trigger, callback_t callback)
{
	trigger->callback = callback;
}

void hooked_callback(u_char* args, int num)
{
	struct trigger *trigger = (struct trigger*)args;
	struct list_head* current;
	struct list_head* head = get_plugins_head(trigger->plug_mgr);
 	struct plugin_node* curr_plugin;
 	int skip_later = 0;
	
	//for each pre-hook
	list_for_each(current, head) {
		curr_plugin = list_entry(current, struct plugin_node, list);
		skip_later = curr_plugin->plugin->pre_hook((void*)trigger, num);
		if (skip_later) {
			printf("plugin '%s' pre_hook return skip_later code\n",
					curr_plugin->plugin->name);
			break;
		}
	}
	// registered callback
	if (!skip_later) {
		trigger->callback(args, num);
	}
	skip_later = 0;
	//for each post-hook
	list_for_each(current, head) {
		curr_plugin = list_entry(current, struct plugin_node, list);
		curr_plugin->plugin->post_hook((void*)trigger, num);
		if (skip_later) {
			printf("plugin '%s' post_hook return skip_later code\n",
					curr_plugin->plugin->name);
			break;
		}
	}
}

int loop(struct trigger *trigger)
{
	int i = trigger->initial_num;
	struct list_head* current;
	struct list_head* head = get_plugins_head(trigger->plug_mgr);
 	struct plugin_node* curr_plugin;
	
	// for each init-hook
	list_for_each(current, head) {
		curr_plugin = list_entry(current, struct plugin_node, list);
		curr_plugin->plugin->init_hook((void*)trigger);
	}

	while (true) {
		sleep(5);
		hooked_callback((u_char*)trigger, i);
		i++;
	}

	// for each exit-hook
	list_for_each(current, head) {
		curr_plugin = list_entry(current, struct plugin_node, list);
		curr_plugin->plugin->exit_hook((void*)trigger);
	}
}

int run_trigger(struct trigger* trigger)
{
	int ret = -1;

	trigger->plug_mgr = load_plugins(trigger->args.plugins_dir);
	if (!trigger->plug_mgr){
		fprintf(stderr, "failed loading plugins\n");
		goto plugin_error;
	}
	if(!loop(trigger)) {
		fprintf(stderr, "loop failed with error\n");
		goto loop_error;
	}
	ret = 0;

loop_error:
	unload_plugins(trigger->plug_mgr);
plugin_error:
	return ret;
}