/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 * 
 *     Copyright (c) 2021 Kalycito Infotech Private Limited (Author: Jayanth Velusamy)
 *
*/

#include <open62541/types.h>
#include <open62541/plugin/log_stdout.h>
#include "ua_server_internal.h"

#ifdef UA_ENABLE_FILETYPE_OBJECT_SUPPORT

#define FILE_CLOSE_STRING                                "Close"
#define FILE_GETPOSITION_STRING                          "GetPosition"
#define FILE_OPEN_STRING                                 "Open"
#define FILE_READ_STRING                                 "Read"
#define FILE_SETPOSITION_STRING                          "SetPosition"
#define FILE_WRITE_STRING                                "Write"
#define FILE_OPENCOUNT_STRING                            "OpenCount"
#define FILE_SIZE_STRING                                 "Size"
#define FILE_WRITABLE_STRING                             "Writable"
#define FILE_USERWRITABLE_STRING                         "UserWritable"

#define STATIC_QN(name) {0, UA_STRING_STATIC(name)}
static const UA_QualifiedName fieldCloseQN = STATIC_QN(FILE_CLOSE_STRING);
static const UA_QualifiedName fieldGetPositionQN = STATIC_QN(FILE_GETPOSITION_STRING);
static const UA_QualifiedName fieldOpenQN = STATIC_QN(FILE_OPEN_STRING);
static const UA_QualifiedName fieldReadQN = STATIC_QN(FILE_READ_STRING);
static const UA_QualifiedName fieldSetPositionQN = STATIC_QN(FILE_SETPOSITION_STRING);
static const UA_QualifiedName fieldWriteQN = STATIC_QN(FILE_WRITE_STRING);
static const UA_QualifiedName fieldOpenCountQN = STATIC_QN(FILE_OPENCOUNT_STRING);
static const UA_QualifiedName fieldSizeQN = STATIC_QN(FILE_SIZE_STRING);
static const UA_QualifiedName fieldWritableQN = STATIC_QN(FILE_WRITABLE_STRING);
static const UA_QualifiedName fieldUserWritableQN = STATIC_QN(FILE_USERWRITABLE_STRING);

/* Gets the NodeId of a Field (e.g. Open) */
static UA_StatusCode
getFieldNodeId(UA_Server *server, const UA_NodeId *conditionNodeId,
                        const UA_QualifiedName* fieldName, UA_NodeId *outFieldNodeId) {
    UA_BrowsePathResult bpr =
        UA_Server_browseSimplifiedBrowsePath(server, *conditionNodeId, 1, fieldName);
    if(bpr.statusCode != UA_STATUSCODE_GOOD)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Parent NodeId not found!");
        return bpr.statusCode;
    }
    UA_StatusCode retval = UA_NodeId_copy(&bpr.targets[0].targetId.nodeId, outFieldNodeId);
    UA_BrowsePathResult_clear(&bpr);
    return retval;
}

static UA_StatusCode
updateOpenCountProperty(UA_Server *server, UA_FileType* fileObject) {
    return UA_Server_writeObjectProperty_scalar(server, fileObject->fileNodeId,
                                                fieldOpenCountQN, &fileObject->openCount,
                                                &UA_TYPES[UA_TYPES_UINT16]);
}

static UA_StatusCode
updateFileSizeProperty(UA_Server *server, UA_FileType* fileObject) {
    UA_file *file = UA_file_open((char*)fileObject->filePath.data, "rb"); //TODO To handle path combination
    if(!file)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error occured while opening the file");
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_file_seek(file, 0, UA_file_seek_end);
    UA_UInt64 fileSize = (UA_UInt64)UA_file_tell(file);
    fileObject->fileSize = fileSize;
    UA_file_close(file);
    return UA_Server_writeObjectProperty_scalar(server, fileObject->fileNodeId,
                                                  fieldSizeQN, &fileObject->fileSize,
                                                  &UA_TYPES[UA_TYPES_UINT64]);
}

static UA_StatusCode
setFileObjectNodeProperties(UA_Server *server, UA_FileType* fileObject) {
    UA_StatusCode retval;
    retval = updateOpenCountProperty(server, fileObject);
    retval |= updateFileSizeProperty(server, fileObject);
    retval |= UA_Server_writeObjectProperty_scalar(server, fileObject->fileNodeId,
                                                  fieldWritableQN, &fileObject->writable,
                                                  &UA_TYPES[UA_TYPES_BOOLEAN]);

    retval |= UA_Server_writeObjectProperty_scalar(server, fileObject->fileNodeId,
                                                  fieldUserWritableQN, &fileObject->userWritable,
                                                  &UA_TYPES[UA_TYPES_BOOLEAN]);
    return retval;
}

static UA_FileType*
getFileTypeObject(UA_Server *server, const UA_NodeId *fileNode) {
    UA_FileType *fileObject = NULL;
    LIST_FOREACH(fileObject, &server->fileObjects, listEntry)
    {
        if(UA_NodeId_equal(&fileObject->fileNodeId, fileNode))
        {
            return fileObject;
        }
    }
    return NULL;
}

static UA_FileInfo*
getFileHandleInfo(UA_Server *server, UA_FileType *fileTypeNode, UA_UInt32 fileHandle) {
    UA_FileInfo* fileInfo = NULL;
    LIST_FOREACH(fileInfo, &fileTypeNode->fileInfo, listEntry)
    {
        if(fileInfo->filehandle == fileHandle)
        {
            return fileInfo;
        }
    }
    return NULL;
}

static UA_file*
getOpenFileResult(UA_FileType *fileObject, UA_Byte fileOpenMode) {
    UA_file *file = NULL;

    if(fileOpenMode == UA_OPENFILEMODE_READ)
    {
        fileObject->filePath.data[fileObject->filePath.length] = '\0';
        file = UA_file_open((char *)fileObject->filePath.data, "rb");
        if(!file)
        {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error occured while opening the file");
            return UA_STATUSCODE_BADNOTFOUND;
        }
        return file;
    }

    if(fileOpenMode == UA_OPENFILEMODE_WRITE)
    {
        fileObject->filePath.data[fileObject->filePath.length] = '\0';
        file = UA_file_open((char *)fileObject->filePath.data, "wb");
        if(!file)
        {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error occured while opening the file");
            return UA_STATUSCODE_BADNOTFOUND;
        }
        return file;
    }

    if((fileOpenMode == UA_OPENFILEMODE_ERASEEXISTING) ||
        (fileOpenMode == (UA_OPENFILEMODE_ERASEEXISTING | UA_OPENFILEMODE_WRITE)))
    {
        fileObject->filePath.data[fileObject->filePath.length] = '\0';
        file = UA_file_open((char *)fileObject->filePath.data, "wb+");
        if(!file)
        {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error occured while opening the file");
            return UA_STATUSCODE_BADNOTFOUND;
        }
        return file;
    }

    if(fileOpenMode == UA_OPENFILEMODE_APPEND)
    {
        fileObject->filePath.data[fileObject->filePath.length] = '\0';
        file = UA_file_open((char *)fileObject->filePath.data, "a");
        if(!file)
        {
            UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error occured while opening the file");
            return UA_STATUSCODE_BADNOTFOUND;
        }
        return file;
    }
    return file;
}

static UA_StatusCode
UA_Server_openFileCallback(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionHandle,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *objectId, void *objectContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    UA_Byte fileOpenMode = *(UA_Byte *)input[0].data;
    UA_StatusCode retval;
    UA_FileType *fileObject = getFileTypeObject(server, objectId);
    if(!fileObject)
    {
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_file *file = getOpenFileResult(fileObject, fileOpenMode);
    if(!file)
    {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    fileObject->openCount++;
    UA_UInt32 fileHandle = (UA_UInt32)UA_file_no(file);
    if(fileHandle < 1)
    {
        return UA_STATUSCODE_BADUNEXPECTEDERROR;
    }
    UA_FileInfo *fileInfo = (UA_FileInfo*) UA_malloc(sizeof(UA_FileInfo));
    fileInfo->file = file;
    fileInfo->filehandle = fileHandle;
    fileInfo->openFileMode = fileOpenMode;
    LIST_INSERT_HEAD(&fileObject->fileInfo, fileInfo, listEntry);
    retval = updateOpenCountProperty(server, fileObject);
    retval |= updateFileSizeProperty(server, fileObject);
    retval |= UA_Variant_setScalarCopy(output, &fileHandle, &UA_TYPES[UA_TYPES_UINT32]);
    return retval;
}

static UA_StatusCode
UA_Server_closeFileCallback(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionHandle,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *objectId, void *objectContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    UA_UInt32 fileHandle = *(UA_UInt32 *)input[0].data;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_FileType *fileObject = getFileTypeObject(server, objectId);
    if(!fileObject)
    {
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_FileInfo *fileInfo = getFileHandleInfo(server, fileObject, fileHandle);
    if(!fileInfo)
    {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    UA_file_close(fileInfo->file);
    LIST_REMOVE(fileInfo, listEntry);
    fileObject->openCount--;
    UA_free(fileInfo);
    retval |= updateOpenCountProperty(server, fileObject);
    retval |= updateFileSizeProperty(server, fileObject);
    return retval;
}

static UA_StatusCode
UA_Server_readFileCallback(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionHandle,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *objectId, void *objectContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    UA_UInt32 fileHandle = *(UA_UInt32 *)input[0].data;
    UA_Int32 value = *(UA_Int32 *)input[1].data;
    size_t requestedLengthToRead = (size_t)value;
    size_t lengthToRead;
    UA_FileType *fileObject = getFileTypeObject(server, objectId);
    if(!fileObject)
    {
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_FileInfo *fileInfo = getFileHandleInfo(server, fileObject, fileHandle);
    if(!fileInfo)
    {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    if(fileInfo->openFileMode != UA_OPENFILEMODE_READ)
    {
        return UA_STATUSCODE_BADINVALIDSTATE;
    }
    UA_ByteString *readBuffer = UA_ByteString_new();
    UA_ByteString_init(readBuffer);
    if(!UA_file_eof(fileInfo->file))
    {
        if(requestedLengthToRead < fileObject->fileSize)
        {
            lengthToRead = requestedLengthToRead;
        }
        else
        {
            lengthToRead = fileObject->fileSize;
        }
        readBuffer->length = lengthToRead;
        readBuffer->data = (UA_Byte*) UA_malloc(readBuffer->length * sizeof(UA_Byte));
        UA_file_read(readBuffer->data, (readBuffer->length + 1), 1, fileInfo->file);
    }
    UA_Variant_setScalarCopy(output, readBuffer, &UA_TYPES[UA_TYPES_BYTESTRING]);
    UA_ByteString_delete(readBuffer);
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
UA_Server_writeFileCallback(UA_Server *server,
                            const UA_NodeId *sessionId, void *sessionHandle,
                            const UA_NodeId *methodId, void *methodContext,
                            const UA_NodeId *objectId, void *objectContext,
                            size_t inputSize, const UA_Variant *input,
                            size_t outputSize, UA_Variant *output) {
    UA_UInt32 fileHandle = *(UA_UInt32 *)input[0].data;
    UA_ByteString *fileContentToWrite = (UA_ByteString*)input[1].data;
    UA_StatusCode retval = UA_STATUSCODE_GOOD;
    UA_FileType *fileObject = getFileTypeObject(server, objectId);
    if(!fileObject)
    {
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_FileInfo *fileInfo = getFileHandleInfo(server, fileObject, fileHandle);
    if(!fileInfo)
    {
        return UA_STATUSCODE_BADINVALIDARGUMENT;
    }
    if(((fileInfo->openFileMode != UA_OPENFILEMODE_ERASEEXISTING) ||
       (fileInfo->openFileMode != (UA_OPENFILEMODE_ERASEEXISTING | UA_OPENFILEMODE_WRITE))) &&
       (fileInfo->openFileMode != UA_OPENFILEMODE_READ))
    {
        for(size_t i = 0; i < fileContentToWrite->length; i++)
        {
            UA_file_print(fileInfo->file, "%c", fileContentToWrite->data[i]);
        }
        return retval;
    }
    retval = UA_STATUSCODE_BADINVALIDSTATE;
    return retval;
}

static UA_StatusCode
UA_Server_setPositionFileCallback(UA_Server *server,
                                    const UA_NodeId *sessionId, void *sessionHandle,
                                    const UA_NodeId *methodId, void *methodContext,
                                    const UA_NodeId *objectId, void *objectContext,
                                    size_t inputSize, const UA_Variant *input,
                                    size_t outputSize, UA_Variant *output) {
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

static UA_StatusCode 
UA_Server_getPositionFileCallback(UA_Server *server,
                                    const UA_NodeId *sessionId, void *sessionHandle,
                                    const UA_NodeId *methodId, void *methodContext,
                                    const UA_NodeId *objectId, void *objectContext,
                                    size_t inputSize, const UA_Variant *input,
                                    size_t outputSize, UA_Variant *output) {
    return UA_STATUSCODE_BADNOTIMPLEMENTED;
}

UA_StatusCode
setFileMethodCallbacks(UA_Server *server, const UA_NodeId fileNodeId) {
    UA_StatusCode result = UA_STATUSCODE_GOOD;
    UA_NodeId openNodeId;
    UA_NodeId readNodeId;
    UA_NodeId setPositionNodeId;
    UA_NodeId getPositionNodeId;
    UA_NodeId writeNodeId;
    UA_NodeId closeNodeId;

    result |= getFieldNodeId(server, &fileNodeId, &fieldOpenQN, &openNodeId);
    result |= getFieldNodeId(server, &fileNodeId, &fieldReadQN, &readNodeId);
    result |= getFieldNodeId(server, &fileNodeId, &fieldSetPositionQN, &setPositionNodeId);
    result |= getFieldNodeId(server, &fileNodeId, &fieldGetPositionQN, &getPositionNodeId);
    result |= getFieldNodeId(server, &fileNodeId, &fieldWriteQN, &writeNodeId);
    result |= getFieldNodeId(server, &fileNodeId, &fieldCloseQN, &closeNodeId);

    result |= UA_Server_setMethodNode_callback(server, openNodeId, UA_Server_openFileCallback);
    result |= UA_Server_setMethodNode_callback(server, readNodeId, UA_Server_readFileCallback);
    result |= UA_Server_setMethodNode_callback(server, setPositionNodeId, UA_Server_setPositionFileCallback);
    result |= UA_Server_setMethodNode_callback(server, getPositionNodeId, UA_Server_getPositionFileCallback);
    result |= UA_Server_setMethodNode_callback(server, writeNodeId, UA_Server_writeFileCallback);
    result |= UA_Server_setMethodNode_callback(server, closeNodeId, UA_Server_closeFileCallback);

    return result;
}

static void
fileTypeObjectDestructor(UA_Server *server,
                    const UA_NodeId *sessionId, void *sessionContext,
                    const UA_NodeId *typeId, void *typeContext,
                    const UA_NodeId *nodeId, void **nodeContext) {
                        printf("Test dstr\n");
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_USERLAND, "FileType object destructor called");
    UA_FileType *fileObject, *fileObject_tmp;
    LIST_FOREACH_SAFE(fileObject, &server->fileObjects, listEntry, fileObject_tmp)
    {
        UA_NodeId_clear(&fileObject->fileNodeId);
        UA_String_clear(&fileObject->filePath);
        UA_FileInfo *fileInfo, *fileInfo_tmp;
        LIST_FOREACH_SAFE(fileInfo, &fileObject->fileInfo, listEntry, fileInfo_tmp)
        {
            UA_file_close(fileInfo->file);
            LIST_REMOVE(fileInfo, listEntry);
            UA_free(fileInfo);
        }
        LIST_REMOVE(fileObject, listEntry);
        UA_free(fileObject);
    }
}


static UA_StatusCode
addFileTypeObjectDestructor(UA_Server *server) {
    UA_NodeTypeLifecycle fileTypeObjectLifecycle;
    fileTypeObjectLifecycle.constructor = NULL;
    fileTypeObjectLifecycle.destructor = fileTypeObjectDestructor;
    UA_StatusCode result = UA_Server_setNodeTypeLifecycle(server, 
                                                          UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE),
                                                          fileTypeObjectLifecycle);
    return result;
}

UA_StatusCode
setFileTypeInfo(UA_Server *server, const UA_NodeId fileNodeId,
                          UA_String filePath) {
    UA_FileType *fileTypeObject = (UA_FileType*) UA_malloc(sizeof(UA_FileType));
    UA_NodeId_copy(&fileNodeId, &fileTypeObject->fileNodeId);
    UA_String_copy(&filePath, &fileTypeObject->filePath);
    fileTypeObject->openCount = 0;
    fileTypeObject->fileSize = 0;
    fileTypeObject->writable = UA_TRUE;
    fileTypeObject->userWritable = UA_TRUE;
    setFileObjectNodeProperties(server, fileTypeObject);
    LIST_INIT(&fileTypeObject->fileInfo);
    LIST_INSERT_HEAD(&server->fileObjects, fileTypeObject, listEntry);
    UA_StatusCode result = addFileTypeObjectDestructor(server);
    printf("The dstr = %x\n", result);
    return UA_STATUSCODE_GOOD;
}

UA_StatusCode 
UA_Server_addFileNode(UA_Server *server, const UA_NodeId requestedNewNodeId,
                       const UA_NodeId parentNodeId,
                       const UA_NodeId referenceTypeId,
                       const UA_QualifiedName browseName,
                       const UA_ObjectAttributes attr,
                       const UA_String filePath,
                       void *nodeContext, UA_NodeId *outNewNodeId) {
    UA_StatusCode retval = UA_Server_addObjectNode(server, requestedNewNodeId,
                            parentNodeId, referenceTypeId, browseName,
                            UA_NODEID_NUMERIC(0, UA_NS0ID_FILETYPE),
                            attr, nodeContext, outNewNodeId);
    
    if(retval != UA_STATUSCODE_GOOD){
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, 
                     "Adding file object node failed with error code %s",
                     UA_StatusCode_name(retval));
        return retval;
    }
    
    UA_file *file = UA_file_open((char*)filePath.data, "w");
    if(!file)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error occured while opening the file");
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_file_close(file);

    //If NodeId is assigned by server
    if(outNewNodeId)
    {
        retval |= setFileTypeInfo(server, *outNewNodeId, filePath);
        retval |= setFileMethodCallbacks(server, *outNewNodeId);
    }

    //else use the  NodeId requested by user
    else
    {
        retval |= setFileTypeInfo(server, requestedNewNodeId, filePath);
        retval |= setFileMethodCallbacks(server, requestedNewNodeId);
    }

    return retval;
}

UA_StatusCode
UA_Server_registerFileNode(UA_Server *server, UA_NodeId fileTypeNodeId,
                           const UA_String filePath) {
    UA_StatusCode result = UA_STATUSCODE_GOOD;
    
    UA_file *initFile = UA_file_open((char*)filePath.data, "w");
    if(!initFile)
    {
        UA_LOG_ERROR(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "Error occured while opening the file");
        return UA_STATUSCODE_BADNOTFOUND;
    }
    UA_file_close(initFile);
    
    result |= setFileTypeInfo(server, fileTypeNodeId, filePath);
    result |= setFileMethodCallbacks(server, fileTypeNodeId);

    return result;
}

#endif
