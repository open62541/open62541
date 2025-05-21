//
// Copyright 2020 NanoMQ Team, Inc. <jaylin@emqx.io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include "nng/nng.h"
#include "nng/supplemental/util/platform.h"
#include "nng/supplemental/nanolib/conf.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HTTP_DEFAULT_USER "admin"
#define HTTP_DEFAULT_PASSWORD "public"
#define HTTP_DEFAULT_PORT 8081
#define HTTP_DEFAULT_ADDR "0.0.0.0"

extern int  start_rest_server(conf *conf);
extern void stop_rest_server(void);

extern void              set_http_server_conf(conf_http_server *conf);
extern conf_http_server *get_http_server_conf(void);

extern void     set_global_conf(conf *config);
extern conf *   get_global_conf(void);
extern char *   get_jwt_key(void);
extern nng_time get_boot_time(void);
#endif
