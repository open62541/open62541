#include <open62541/server.h>

int main(void) {
    UA_Server *server = UA_Server_new();

    // INCULDE FILE SYSTEM LOGIC HERE

    UA_Server_runUntilInterrupt(server);
    UA_Server_delete(server);
    return 0;
}