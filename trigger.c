#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include "plugin_manager.h"
#include "trigger.h"
#include "list.h"

void hook_callback(struct trigger *trigger, callback_t callback, void* args, void* data)
{
	struct list_head* current;
	struct list_head* head = get_plugins_head(trigger->plug_mgr);
	struct plugin_node* curr_plugin;
	int skip_later = 0;

	//for each pre-hook
	list_for_each(current, head) {
		curr_plugin = list_entry(current, struct plugin_node, list);
		skip_later = curr_plugin->plugin->pre_hook(args, data);
		if (skip_later) {
			printf("plugin '%s' pre_hook return skip_later code\n",
					curr_plugin->plugin->name);
			break;
		}
	}

	// registered callback
	if (!skip_later) {
		callback(args, data);
	}
	skip_later = 0;

	//for each post-hook
	list_for_each(current, head) {
		curr_plugin = list_entry(current, struct plugin_node, list);
		curr_plugin->plugin->post_hook(args, data);
		if (skip_later) {
			printf("plugin '%s' post_hook return skip_later code\n",
					curr_plugin->plugin->name);
			break;
		}
	}
}

void register_loop(struct trigger *trigger, loop_t loop)
{
	trigger->loop = loop;
}

void hooked_loop(struct trigger *trigger, void *args)
{
	struct list_head* current;
	struct list_head* head = get_plugins_head(trigger->plug_mgr);
	struct plugin_node* curr_plugin;

	// for each init-hook
	list_for_each(current, head) {
		curr_plugin = list_entry(current, struct plugin_node, list);
		curr_plugin->plugin->init_hook((void*)trigger);
	}

	trigger->loop(args);

	// for each exit-hook
	list_for_each(current, head) {
		curr_plugin = list_entry(current, struct plugin_node, list);
		curr_plugin->plugin->exit_hook((void*)trigger);
	}
}

int run_trigger(struct trigger* trigger, void *args)
{
	int ret = -1;

	trigger->plug_mgr = load_plugins(trigger->plugins_dir);
	if (!trigger->plug_mgr){
		fprintf(stderr, "failed loading plugins\n");
		goto plugin_error;
	}
	hooked_loop(trigger, args);

	unload_plugins(trigger->plug_mgr);
	ret = 0;

plugin_error:
	return ret;
}
