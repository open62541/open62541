/*
 * Copyright (C) 2014 the contributors as stated in the AUTHORS file
 *
 * This file is part of open62541. open62541 is free software: you can
 * redistribute it and/or modify it under the terms of the GNU Lesser General
 * Public License, version 3 (as published by the Free Software Foundation) with
 * a static linking exception as stated in the LICENSE file provided with
 * open62541.
 *
 * open62541 is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 */

#ifndef UA_NODESTORE_STANDARD_H_
#define UA_NODESTORE_STANDARD_H_

#include "ua_nodestore_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Instanciate new NodeStoreInterface for this nodestore, as the open62541 standard nodestore*/
UA_EXPORT UA_NodestoreInterface UA_Nodestore_standard(void);

/* Delete the new NodeStoreInterface for the open62541 standard nodestore*/
UA_EXPORT void UA_Nodestore_standard_delete(UA_NodestoreInterface * nsi);

#ifdef __cplusplus
} // extern "C"
#endif

#endif /* UA_NODESTORE_STANDARD_H_ */
