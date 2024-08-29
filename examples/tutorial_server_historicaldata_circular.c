/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 * Copyright 2019 (c) basysKom GmbH <opensource@basyskom.com> (Author: Peter Rustler)
 * Copyright 2021 (c) luibass92 <luibass92@live.it> (Author: Luigi Bassetta)
 */

#include <open62541/plugin/historydata/history_data_backend_memory.h>
#include <open62541/plugin/historydata/history_data_gathering_default.h>
#include <open62541/plugin/historydata/history_database_default.h>
#include <open62541/plugin/historydatabase.h>
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>

int main(void) {
    UA_Server *server = UA_Server_new();
    UA_ServerConfig *config = UA_Server_getConfig(server);

    /* We need a gathering for the plugin to constuct.
     * The UA_HistoryDataGathering is responsible to collect data and store it to the database.
     * We will use this gathering for one node, only. initialNodeIdStoreSize = 1
     * The store will NOT automatically grow if you register more than one node will return a UA_STATUS_BADOUTOFMEMORY. */
    UA_HistoryDataGathering gathering = UA_HistoryDataGathering_Circular(1);

    /* We set the responsible plugin in the configuration. UA_HistoryDatabase is
     * the main plugin which handles the historical data service. */
    config->historyDatabase = UA_HistoryDatabase_default(gathering);

    /* Define the attribute of the uint32 variable node */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_UInt32 myUint32 = 40;
    UA_Variant_setScalar(&attr.value, &myUint32, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US","myUintValue");
    attr.displayName = UA_LOCALIZEDTEXT("en-US","myUintValue");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    /* We set the access level to also support history read
     * This is what will be reported to clients */
    attr.accessLevel =
        UA_ACCESSLEVELMASK_READ | UA_ACCESSLEVELMASK_WRITE | UA_ACCESSLEVELMASK_HISTORYREAD;
    /* We also set this node to historizing, so the server internals also know from it. */
    attr.historizing = true;

    /* Add the variable node to the information model */
    UA_NodeId uint32NodeId = UA_NODEID_STRING(1, "myUintValue");
    UA_QualifiedName uint32Name = UA_QUALIFIEDNAME(1, "myUintValue");
    UA_NodeId parentNodeId = UA_NS0ID(OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NS0ID(ORGANIZES);
    UA_NodeId outNodeId;
    UA_NodeId_init(&outNodeId);
    UA_StatusCode retval =
        UA_Server_addVariableNode(server, uint32NodeId, parentNodeId,
                                  parentReferenceNodeId, uint32Name,
                                  UA_NS0ID(BASEDATAVARIABLETYPE),
                                  attr, NULL, &outNodeId);

    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "UA_Server_addVariableNode %s", UA_StatusCode_name(retval));

    /* Now we define the settings for our node */
    UA_HistorizingNodeIdSettings setting;

    /* There is a memory based database plugin. We will use that. We just
     * reserve space for 3 nodes with 10 values each. This will NOT automatically grow
     * but will store data as a circular buffer of size 10. The 11th value will be
     * stored replacing the oldest one and the process will continue like that. */
    setting.historizingBackend = UA_HistoryDataBackend_Memory_Circular(3, 10);

    /* We want the server to serve a maximum of 100 values per request. This
     * value depend on the plattform you are running the server. A big server
     * can serve more values, smaller ones less. */
    setting.maxHistoryDataResponseSize = 100;

    /* If we have a sensor which do not report updates
     * and need to be polled we change the setting like that.
     * The polling interval in ms.
     *
    setting.pollingInterval = 100;
     *
     * Set the update strategie to polling.
     *
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_POLL;
     */

    /* If you want to insert the values to the database yourself, we can set the user strategy here.
     * This is useful if you for example want a value stored, if a defined delta is reached.
     * Then you should use a local monitored item with a fuzziness and store the value in the callback.
     *
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_USER;
     */

    /* We want the values stored in the database, when the nodes value is
     * set. */
    setting.historizingUpdateStrategy = UA_HISTORIZINGUPDATESTRATEGY_VALUESET;

    /* At the end we register the node for gathering data in the database. */
    retval = gathering.registerNodeId(server, gathering.context, &outNodeId, setting);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER,
                "registerNodeId %s", UA_StatusCode_name(retval));

    /* If you use UA_HISTORIZINGUPDATESTRATEGY_POLL, then start the polling.
     *
    retval = gathering.startPoll(server, gathering.context, &outNodeId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "startPoll %s", UA_StatusCode_name(retval));
    */

    UA_Server_runUntilInterrupt(server);

    /*
     * If you use UA_HISTORIZINGUPDATESTRATEGY_POLL, then stop the polling.
     *
    retval = gathering.stopPoll(server, gathering.context, &outNodeId);
    UA_LOG_INFO(UA_Log_Stdout, UA_LOGCATEGORY_SERVER, "stopPoll %s", UA_StatusCode_name(retval));
    */

    UA_Server_delete(server);

    setting.historizingBackend.deleteMembers(&setting.historizingBackend);
    UA_NodeId_clear(&outNodeId);
    return 0;
}
