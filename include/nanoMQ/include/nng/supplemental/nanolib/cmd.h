//
// Copyright 2020 NanoMQ Team, Inc. <jaylin@emqx.io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//
#include <sys/types.h>

#define CMD_BUFF_LEN 1024
extern char *cmd_output_buff;
extern int   cmd_output_len;

#ifndef NNG_PLATFORM_WINDOWS

#define CMD_RUN(cmd)                \
	do {                        \
		ret = cmd_run(cmd); \
		if (ret < 0)        \
			goto err;   \
	} while (0)

#define CMD_FRUN(fcmd, ...)                             \
	do {                                            \
		ret = nano_cmd_frun(fcmd, __VA_ARGS__); \
		if (ret < 0)                            \
			goto err;                       \
	} while (0)

extern int nano_cmd_run(const char *cmd);
extern int nano_cmd_run_status(const char *cmd);
extern int nano_cmd_frun(const char *format, ...);

#endif

extern void nano_cmd_cleanup(void);
