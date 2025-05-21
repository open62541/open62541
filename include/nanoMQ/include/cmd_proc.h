#ifndef NANOMQ_CMD_PROC_H
#define NANOMQ_CMD_PROC_H

#define CMD_IPC_URL "ipc:///tmp/nanomq_cmd.ipc"
#define IPC_URL_PATH "/tmp/nanomq_cmd.ipc"
// #define CMD_IPC_URL "tcp://127.0.0.1:10000"
#define CMD_PROC_PARALLEL 1

#include "nng/nng.h"
#include "nng/supplemental/nanolib/conf.h"
#include "nng/supplemental/nanolib/log.h"

typedef struct cmd_work cmd_work;

extern void      cmd_server_cb(void *arg);
extern cmd_work *alloc_cmd_work(nng_socket sock, conf *config);
extern void      start_cmd_client(const char *cmd, const char *url);
extern char *    encode_client_cmd(const char *conf_file, int type);

#endif // NANOMQ_CMD_PROC_H
