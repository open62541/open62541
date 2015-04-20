#ifndef TESTING_NETWORKLAYERS_H_
#define TESTING_NETWORKLAYERS_H_

#ifdef NOT_AMALGATED
# include "ua_server.h"
#else
# include "open62541.h"
#endif

/** @brief Create the TCP networklayer and listen to the specified port */
UA_ServerNetworkLayer
ServerNetworkLayerFileInput_new(UA_UInt32 files, char **filenames, void(*readCallback)(void),
                                void(*writeCallback) (void*, UA_ByteStringArray buf),
                                void *callbackHandle);

#endif /* TESTING_NETWORKLAYERS_H_ */
