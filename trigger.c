#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "plugin_manager.h"
#include "trigger.h"
#include "list.h"

/* Structure that defines a hash table [name]<->[callback]*/
struct callbacks_table {
	char name[MAX_HOOK_NAME];
	callback_t callback;
	UT_hash_handle hh;
};

static callback_t get_callback(struct trigger *trigger, const char *name)
{
	struct callbacks_table *item;

	HASH_FIND_STR(trigger->callbacks, name, item);
	if (!item) {
		return NULL;
	}
	return item->callback;
}

static int set_callback(struct trigger *trigger, const char *name, callback_t callback)
{
	struct callbacks_table *item;

	if (get_callback(trigger, name)) {
		return EEXIST;
	}
	item = (struct callbacks_table *)calloc(1, sizeof(struct callbacks_table));
	if (!item) {
		fprintf(stderr, "malloc callbacks_table: %m\n");
		return errno;
	}
	strncpy(item->name, name, MAX_HOOK_NAME);
	item->name[MAX_HOOK_NAME - 1] = '\0';
	item->callback = callback;

	/* In the below macro the argument 'name' is the key field in struct*/
	HASH_ADD_STR(trigger->callbacks, name, item);

	return 0;
}

static int run_hooks(struct list_head *head, int post_hook, void* args, void* data)
{
	struct module_hooks_node* curr_hook;
	struct list_head* current;
	int ret = 0;

	/* No modules where registered */
	if (!head) { 
		return ENOENT;
	}

	list_for_each(current, head) {
		curr_hook = list_entry(current, struct module_hooks_node, list);
		if (post_hook) {
			ret = curr_hook->m_hook->post_hook(args, data);
		} else {
			ret = curr_hook->m_hook->pre_hook(args, data);
		}
		if (ret) {
			goto skip_later;	
		}
	}

skip_later:
	return ret;
}

static void hooked_loop(struct trigger *trigger, void *args)
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

int register_callback(struct trigger *trigger, const char *name, callback_t callback)
{
	int ret = 0;

	ret = register_module(trigger->plug_mgr, name);
	if (ret) {
		printf("Could not register plugin module.");
		goto error_register;
	}
	ret = set_callback(trigger, name, callback);
	if (ret) {
		printf("Could not register callback for module.");
	}

error_register:
	return ret;
}


int handle_callback(struct trigger *trigger, const char* name, void* args, void* data)
{
	struct list_head* head = get_module_hooks(trigger->plug_mgr, name);
	callback_t callback = 0;
 	int ret = -1;

	callback = get_callback(trigger, name);
	if (!callback) {
		printf("module '%s' not registered.\n", name);
		ret = ENOENT;
		goto error_no_callback;
	}

	ret = run_hooks(head, 0, args, data);
	if (!ret) {
		ret = callback(args, data);
	}
	ret = run_hooks(head, 1, args, data);

error_no_callback:
	return ret;
}

void register_loop(struct trigger *trigger, loop_t loop)
{
	trigger->loop = loop;
}

int init_trigger(struct trigger* trigger, const char *path)
{
	trigger->callbacks = NULL;
	trigger->plug_mgr = init_plugin_manager(path);
	if (!trigger->plug_mgr) {
		return -1;
	}
	return 0;
}

void destory_trigger(struct trigger *trigger)
{
	struct callbacks_table *item, *tmp;

	destroy_plugin_manager(trigger->plug_mgr);

	/* free callbacks table */
	HASH_ITER(hh, trigger->callbacks, item, tmp) {
		HASH_DEL(trigger->callbacks, item);
		free(item);
	}
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