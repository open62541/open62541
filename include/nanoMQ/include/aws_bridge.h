#ifndef AWS_BRIDGE_H
#define AWS_BRIDGE_H

#include "nng/supplemental/nanolib/conf.h"
#include "broker.h"

extern int  aws_bridge_client(conf_bridge_node *node);
extern void aws_bridge_forward(nano_work *work);

#endif
