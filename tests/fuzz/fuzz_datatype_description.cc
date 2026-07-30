/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2026 (c) OSS-Fuzz coverage expansion
 */

/*
 * Fuzz target for src/ua_types_definition.c (UA_DataType_fromDescription).
 *
 * OPC UA servers can publish custom DataTypes through the type dictionary as
 * SimpleTypeDescription / EnumDescription / StructureDescription values. A
 * client reading these from a (potentially malicious) server feeds them into
 * UA_DataType_fromDescription(), which materialises an in-memory UA_DataType:
 * it allocates the member array, copies attacker-controlled member names,
 * and computes C-struct sizes / paddings from attacker-controlled fields
 * (fieldsSize, valueRank, arrayDimensions, isOptional, ...).
 *
 * This conversion logic (ua_types_definition.c) is reported as 0% covered in
 * the public OSS-Fuzz coverage report: the existing fuzz_binary_decode harness
 * decodes the description structs but never converts them into a UA_DataType,
 * so none of the size/padding/member-population code is exercised.
 *
 * The harness decodes one of the three description structs from the fuzz input
 * (the description body is itself attacker-controlled wire data), wraps it in
 * an ExtensionObject, and runs it through the public UA_DataType_fromDescription
 * entry point.
 */

#include "custom_memory_manager.h"

#include <open62541/types.h>
#include <open62541/util.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    /* Need 4 bytes for the memory limit + 1 selector byte. */
    if(size <= 5)
        return 0;

    /* Bound total allocations using the trailing 4 bytes, like the other
     * open62541 decode harnesses, so that an attacker-controlled huge
     * fieldsSize does not turn into an OOM false positive. */
    if(!UA_memoryManager_setLimitFromLast4Bytes(data, size))
        return 0;

    /* The first byte selects which description struct to decode. */
    uint8_t sel = data[0] % 3;
    data += 1;
    size -= 1;

    const UA_DataType *descrType;
    switch(sel) {
    case 0:
        descrType = &UA_TYPES[UA_TYPES_STRUCTUREDESCRIPTION];
        break;
    case 1:
        descrType = &UA_TYPES[UA_TYPES_ENUMDESCRIPTION];
        break;
    default:
        descrType = &UA_TYPES[UA_TYPES_SIMPLETYPEDESCRIPTION];
        break;
    }

    void *descrData = UA_new(descrType);
    if(!descrData)
        return 0;

    const UA_ByteString binary = {
        size,                       /* length */
        (UA_Byte *)(void *)data     /* data (read-only) */
    };

    UA_StatusCode ret = UA_decodeBinary(&binary, descrData, descrType, NULL);
    if(ret != UA_STATUSCODE_GOOD) {
        UA_delete(descrData, descrType);
        return 0;
    }

    /* Wrap the decoded description in an ExtensionObject; setValue takes
     * ownership of descrData, so it is freed by the later clear(). */
    UA_ExtensionObject eo;
    UA_ExtensionObject_init(&eo);
    UA_ExtensionObject_setValue(&eo, descrData, descrType);

    /* Exercise the public conversion entry point. */
    UA_DataType type;
    UA_StatusCode r2 = UA_DataType_fromDescription(&type, &eo, NULL);
    if(r2 == UA_STATUSCODE_GOOD) {
        /* Round-trip back into a description to exercise the reverse path. */
        UA_ExtensionObject eo2;
        UA_StatusCode r3 = UA_DataType_toDescription(&type, &eo2);
        if(r3 == UA_STATUSCODE_GOOD)
            UA_ExtensionObject_clear(&eo2);
        UA_DataType_clear(&type);
    }

    UA_ExtensionObject_clear(&eo);
    return 0;
}
