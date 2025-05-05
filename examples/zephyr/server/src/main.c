#include <zephyr/net/net_ip.h>
#include <zephyr/net/net_l2.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/socket.h>
#include <zephyr/net/wifi.h>
#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/wifi_utils.h>
#include <zephyr/sys/sys_heap.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(open62541_test, LOG_LEVEL_DBG);

#include <open62541/server.h>
#include <open62541/server_config_default.h>

int main(void) {
  k_sleep(K_MSEC(1000));
  struct net_if *iface = net_if_get_default();
  struct in_addr addr;
  struct in_addr mask;
  int ret = zsock_inet_pton(AF_INET, "192.0.2.10", &addr);
  if (ret < 0) {
    LOG_ERR("Invalid IP address format");
    return 1;
  }
  ret = zsock_inet_pton(AF_INET, "255.255.255.0", &mask);
  if (ret < 0) {
    LOG_ERR("Invalid IP address format");
    return 1;
  }
  struct net_if_addr *in_addr =
      net_if_ipv4_addr_add(iface, &addr, NET_ADDR_MANUAL, 0);
  net_if_ipv4_set_netmask_by_addr(iface, &addr, &mask);
  if (!in_addr) {
    LOG_ERR("Failed to add IPv4 address");
    return 1;
  }
  k_sleep(K_MSEC(1000));

  LOG_INF("Starting UA_Server");
  static UA_ServerConfig config;
  memset(&config, 0, sizeof(UA_ServerConfig));
  UA_ServerConfig_setDefault(&config);
  // Minimum is 8192
  config.tcpBufSize = 1 << 13;
  config.tcpMaxMsgSize = 1 << 13;
  config.maxSecureChannels = 1;
  config.maxSessions = 1;
  UA_Server *server = UA_Server_newWithConfig(&config);
  if (server == NULL) {
    LOG_ERR("UA_Server_new failed");
    return EXIT_FAILURE;
  }

  // Add a variable node to the adresspace
  UA_VariableAttributes attr; attr = UA_VariableAttributes_default;
  UA_Int32 myInteger; myInteger = 42;
  UA_Variant_setScalarCopy(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
  attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US", "the answer");
  attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US", "the answer");
  UA_NodeId myIntegerNodeId; myIntegerNodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
  UA_QualifiedName myIntegerName; myIntegerName = UA_QUALIFIEDNAME_ALLOC(1, "the answer");
  UA_NodeId parentNodeId; parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
  UA_NodeId parentReferenceNodeId; parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
  UA_Server_addVariableNode(server, myIntegerNodeId, parentNodeId,
                            parentReferenceNodeId, myIntegerName,
                            UA_NODEID_NULL, attr, NULL, NULL);

  // /* Allocations on the heap need to be freed */
  UA_VariableAttributes_clear(&attr);
  UA_NodeId_clear(&myIntegerNodeId);
  UA_QualifiedName_clear(&myIntegerName);

  volatile UA_Boolean running = true;
  UA_StatusCode retval = UA_Server_run(server, &running);

  UA_Server_delete(server);
  return retval == UA_STATUSCODE_GOOD ? EXIT_SUCCESS : EXIT_FAILURE;
}
