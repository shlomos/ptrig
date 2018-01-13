#include "plugin_manager.h"

int init_hook(void* args)
{
	printf("foo\n");
	return 0;
}

int exit_hook(void* args)
{
	printf("bar\n");
	return 0;
}

int pre_hook(u_char* args, int num)
{
	printf("pre_foo_%d\n", num);
	return 0;
}

int post_hook(u_char* args, int num)
{
	printf("post_foo_%d\n", num);
	return 0;
}

struct plugin trigger_plugin_hooks ={
	.name = "foo",
	.init_hook = init_hook,
	.exit_hook = exit_hook,
	.pre_hook = pre_hook,
	.post_hook = post_hook
};