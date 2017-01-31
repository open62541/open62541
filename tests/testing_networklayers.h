#ifndef TESTING_NETWORKLAYERS_H_
#define TESTING_NETWORKLAYERS_H_

#include "ua_server.h"

/** @brief Create the TCP networklayer and listen to the specified port */
UA_Connection createDummyConnection(void);

#endif /* TESTING_NETWORKLAYERS_H_ */
