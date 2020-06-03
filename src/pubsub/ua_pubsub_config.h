#ifdef UA_ENABLE_PUBSUB_CONFIG

#ifndef UA_PUBSUB_CONFIG_H_
#define UA_PUBSUB_CONFIG_H_

#include <open62541/types_generated.h>  /* Defines UA data types */
#include <open62541/server.h>           /* Defines UA_Server     */


/* UA_decodeBinFile() */
/**
 *  @brief      Decodes buffered binary config file
 * 
 *  @param      buffer      [in]    Pointer to buffer that buffers the bin file
 *  @param      offset      [bi]    Offset in the buffer
 *  @param      dst         [out]   Pointer to Object that shall contain the decoded data
 * 
 *  @return     UA_STATUSCODE_GOOD on success
 */
UA_StatusCode 
UA_decodeBinFile
(
    /*[in]*/    const UA_ByteString *buffer, 
    /*[bi]*/    size_t offset,
    /*[out]*/   UA_ExtensionObject *dst
);


/* UA_getPubSubConfig() */
/**
 *  @brief      extracts PubSub Configuration from decoded Extension Object
 * 
 *  @param      src         [in]    Pointer to Decoded source object of type ExtensionObject
 *  @param      dst         [out]   Pointer on PubSub Configuration
 * 
 *  @return     UA_STATUSCODE_GOOD on success
 */
UA_StatusCode 
UA_getPubSubConfig
(
    /*[in]*/    const UA_ExtensionObject *const src, 
    /*[out]*/   UA_PubSubConfigurationDataType **dst
);


/* UA_updatePubSubConfig() */
/**
 *  @brief      Configures a PubSub Server with given PubSubConfigurationDataType object
 * 
 *  @param      server                      [bi]    Pointer to Server object
 *  @param      configurationParameters     [in]    Pointer to PubSubConfiguration object
 * 
 *  @return     UA_STATUSCODE_GOOD on success
 */
UA_StatusCode 
UA_updatePubSubConfig
(
    /*[bi]*/    UA_Server* server, 
    /*[in]*/    const UA_PubSubConfigurationDataType *const configurationParameters
);


/* UA_loadPubSubConfigFromFile() */
/**
 * @brief       Opens file, decodes the information and loads the PubSub configuration into server.
 * 
 * @param       server      [bi]    Pointer to Server object that shall be configured
 * @param       filename    [in]    Relative path and name of the file that contains the PubSub configuration
 * 
 * @return      UA_STATUSCODE_GOOD on success
 */
UA_StatusCode 
UA_loadPubSubConfigFromFile
(
    /*[bi]*/    UA_Server *server, 
    /*[in]*/    const UA_String filename
);


/* UA_writeBufferToFile() */
/**
 * @brief   Writes a buffer (ByteString) to a file in the filesystem.
 * 
 * @param   filename    [in]    Name of the file that shall be created / overwritten.
 * @param   buffer      [in]    ByteString that contains the data that shall be written.
 * 
 * @return  UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_writeBufferToFile
(
    /*[in]*/    const UA_String filename, 
    /*[in]*/    const UA_ByteString buffer
);

/* UA_savePubSubConfigToFile() */
/**
 * @brief   Creates bin file in the filesystem, which contains the current PubSub configuration of the server.
 * @param   server  [in]    Server object whose PubSub configuration shall be written to file.
 * @return  UA_STATUSCODE_GOOD on success
 */
UA_StatusCode
UA_savePubSubConfigToFile
(
    /*[in]*/    UA_Server *server
);

#endif /* UA_PUBSUB_CONFIG_H_ */

#endif /* UA_ENABLE_PUBSUB_CONFIG */
