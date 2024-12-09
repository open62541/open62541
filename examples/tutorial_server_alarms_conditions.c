/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information. */

#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>

/**
 * Using Alarms and Conditions Server
 * ----------------------------------
 *
 * Besides the usage of monitored items and events to observe the changes in the
 * server, it is also important to make use of the Alarms and Conditions Server
 * Model. Alarms are events which are triggered automatically by the server
 * dependent on internal server logic or user specific logic when the states of
 * server components change. The state of a component is represented through a
 * condition. So the values of all the condition children (Fields) are the
 * actual state of the component.
 *
 * Trigger Alarm events by changing States
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * The following example will be based on the server events tutorial. Please
 * make sure to understand the principle of normal events before proceeding with
 * this example! */



static void
setUpEnvironment(UA_Server *server) {
    //TODO
}

/**
 * It follows the main server code, making use of the above definitions. */

int main (void) {


    UA_Server *server = UA_Server_new();

    setUpEnvironment(server);

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}
