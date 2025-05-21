//
// Copyright 2024 NanoMQ Team, Inc. <wangwei@emqx.io>
//
// This software is supplied under the terms of the MIT License, a
// copy of which should be located in the distribution where this
// file was obtained (LICENSE.txt).  A copy of the license may also be
// found online at https://opensource.org/licenses/MIT.
//

#if defined(SUPP_ICEORYX)

#ifndef NNG_ICEORYX_SHM_H
#define NNG_ICEORYX_SHM_H

#ifdef __cplusplus
extern "C" {
#endif

// TODO
#include "../../../src/supplemental/iceoryx/iceoryx_api.h"

struct nng_iceoryx_suber {
	nano_iceoryx_suber *suber;
};
typedef struct nng_iceoryx_suber nng_iceoryx_suber;

struct nng_iceoryx_puber {
	nano_iceoryx_puber *puber;
};
typedef struct nng_iceoryx_puber nng_iceoryx_puber;


NNG_DECL int nng_iceoryx_open(nng_socket *sock, const char *runtimename);

NNG_DECL int nng_iceoryx_sub(nng_socket *sock, const char *subername,
    const char *const service_name, const char *const instance_name,
    const char *const event, nng_iceoryx_suber **nsp);

NNG_DECL int nng_iceoryx_pub(nng_socket *sock, const char *pubername,
    const char *const service_name, const char *const instance_name,
    const char *const event, nng_iceoryx_puber **npp);

NNG_DECL int nng_msg_iceoryx_alloc(nng_msg **msgp, nng_iceoryx_puber *puber, size_t sz);
NNG_DECL int nng_msg_iceoryx_free(nng_msg *msg, nng_iceoryx_suber *suber);
NNG_DECL int nng_msg_iceoryx_append(nng_msg *msg, const void *data, size_t sz);

#ifdef __cplusplus
}
#endif

#endif

#endif

