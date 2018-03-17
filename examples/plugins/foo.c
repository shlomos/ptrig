#include "plugin_manager.h"

struct callback_data {
	int current;
};

int init_hook(void *args)
{
	printf("foo\n");
	return 0;
}

int exit_hook(void *args)
{
	printf("bar\n");
	return 0;
}

int pre_default_hook(void *args, void *data)
{
	struct callback_data *cdata = (struct callback_data *)data;

	printf("pre_foo_%d\n", cdata->current);
	return 0;
}

int post_default_hook(void *args, void *data)
{
	struct callback_data *cdata = (struct callback_data *)data;

	printf("post_foo_%d\n", cdata->current);
	return 0;
}

struct general_hook ptrig_hooks = {
	.name = "foo",
	.init_hook = init_hook,
	.exit_hook = exit_hook
};

struct module_hook ptrig_default_hooks = {
	.pre_hook = pre_default_hook,
	.post_hook = post_default_hook
};
