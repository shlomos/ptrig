#include "plugin_manager.h"

struct callback_data {
	int current;
};

int init_hook(void* args)
{
	printf("hello\n");
	return 0;
}

int exit_hook(void* args)
{
	printf("bye\n");
	return 0;
}

int each_pre_hook(void* args, void* data)
{
	struct callback_data *cdata = (struct callback_data*)data;

	printf("pre_hello_%d\n", cdata->current);
	return 0;
}

int each_post_hook(void* args, void* data)
{
	struct callback_data *cdata = (struct callback_data*)data;

	printf("post_hello_%d\n", cdata->current);
	return 0;
}

int triplets_pre_hook(void* args, void* data)
{
	struct callback_data *cdata = (struct callback_data*)data;

	printf("pre_hello_%d\n", cdata->current);
	return 0;
}

int triplets_post_hook(void* args, void* data)
{
	struct callback_data *cdata = (struct callback_data*)data;

	printf("post_hello_%d\n", cdata->current);
	return 0;
}

struct plugin_hook ptrig_each_plugin_hooks = {
	.pre_hook = each_pre_hook,
	.post_hook = each_post_hook
};

struct plugin_hook ptrig_triplets_plugin_hooks = {
	.pre_hook = triplets_pre_hook,
	.post_hook = triplets_post_hook
};

struct plugin ptrig_plugin_hooks = {
	.name = "hello",
	.init_hook = init_hook,
	.exit_hook = exit_hook,
	.hooks = {ptrig_each_plugin_hooks, ptrig_triplets_plugin_hooks}
};
