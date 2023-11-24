//
// Created by flo47663 on 21.11.2023.
//

#ifndef OPEN62541_EVENTFILTER_PARSER_GRAMMAR_H
#define OPEN62541_EVENTFILTER_PARSER_GRAMMAR_H

#include "open62541/plugin/log_stdout.h"
#include "open62541/server.h"
#include "open62541/server_config_default.h"
#include "open62541_queue.h"
#include "eventfilter_parser.h"
#pragma GCC diagnostic ignored "-Wmissing-prototypes"
#pragma GCC diagnostic ignored "-Wunused-function"
UA_StatusCode UA_EventFilter_parse(UA_ByteString *content, UA_EventFilter *filter);
void clear_event_filter(UA_EventFilter *filter);


#endif  // OPEN62541_EVENTFILTER_PARSER_GRAMMAR_H
