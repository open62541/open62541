//
// Copyright 2024 NanoMQ Team, Inc. <jaylin@emqx.io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
#ifndef NANOMQ_PLUGIN_H
#define NANOMQ_PLUGIN_H

#include "nng/nng.h"
#include "nng/supplemental/nanolib/conf.h"
#include "nng/supplemental/nanolib/log.h"

struct nano_plugin {
	char *path;
	int (*init)(void);
};

enum hook_point {
	HOOK_USER_PROPERTY,
};

struct plugin_hook {
	unsigned int point;
	int (*cb)(void *data);
};

extern int plugin_hook_register(unsigned int point, int (*cb)(void *data));
extern int plugin_hook_call(unsigned int point, void *data);
extern int plugin_register(char *path);
extern void plugins_clear();
extern int plugin_init(struct nano_plugin *plugin);

#endif // NANOMQ_PLUGIN_H
