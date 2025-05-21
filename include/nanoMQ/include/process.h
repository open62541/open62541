#ifndef PROCESS_H
#define PROCESS_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

extern int process_is_alive(int pid);
extern int process_send_signal(int pid, int signal);
extern int pidgrp_send_signal(int pid, int signal);
extern int process_daemonize(void);
extern int process_create_child(int (*child_run)(void *), void *data);

#endif