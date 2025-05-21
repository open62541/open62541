#ifndef WEBHOOK_INPROC_H
#define WEBHOOK_INPROC_H

#include "nng/supplemental/nanolib/conf.h"
#include "nng/nng.h"

#define HOOK_IPC_URL "ipc:///tmp/nanomq_hook.ipc"
#define EXTERNAL2NANO_IPC "EX2NANO"

extern int start_hook_service(conf *conf);
extern int stop_hook_service(void);

#endif
