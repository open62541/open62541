/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"
#include "lwip/opt.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include <open62541/plugin/log_stdout.h>
#include <open62541/server.h>
#include <open62541/server_config_default.h>

#include "lwip/opt.h"

#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "netif/ppp/pppoe.h"

#ifdef USE_OBSOLETE_USER_CODE_SECTION_4
/* Kept to help code migration. (See new 4_1, 4_2... sections) */
/* Avoid to use this user section which will become obsolete. */
/* USER CODE BEGIN 4 */
/* USER CODE END 4 */
#endif

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void opcua_thread(void *arg){

        //The default 64KB of memory for sending and receicing buffer caused problems to many users. With the code below, they are reduced to ~16KB
        UA_UInt32 sendBufferSize = 16000;       //64 KB was too much for my platform
        UA_UInt32 recvBufferSize = 16000;       //64 KB was too much for my platform
        UA_UInt16 portNumber = 4840;

        UA_Server* mUaServer = UA_Server_new();
        UA_ServerConfig *uaServerConfig = UA_Server_getConfig(mUaServer);
        UA_ServerConfig_setMinimalCustomBuffer(uaServerConfig, portNumber, 0, sendBufferSize, recvBufferSize);

        //VERY IMPORTANT: Set the hostname with your IP before starting the server
        //UA_ServerConfig_setCustomHostname(uaServerConfig, UA_STRING("192.168.0.102"));

        //The rest is the same as the example

        UA_Boolean running = true;

        // add a variable node to the adresspace
        UA_VariableAttributes attr = UA_VariableAttributes_default;
        UA_Int32 myInteger = 42;
        UA_Variant_setScalarCopy(&attr.value, &myInteger, &UA_TYPES[UA_TYPES_INT32]);
        attr.description = UA_LOCALIZEDTEXT_ALLOC("en-US","the answer");
        attr.displayName = UA_LOCALIZEDTEXT_ALLOC("en-US","the answer");
        UA_NodeId myIntegerNodeId = UA_NODEID_STRING_ALLOC(1, "the.answer");
        UA_QualifiedName myIntegerName = UA_QUALIFIEDNAME_ALLOC(1, "the answer");
        UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
        UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
        UA_Server_addVariableNode(mUaServer, myIntegerNodeId, parentNodeId,
                                                                parentReferenceNodeId, myIntegerName,
                                                                UA_NODEID_NULL, attr, NULL, NULL);

        /* allocations on the heap need to be freed */
        UA_VariableAttributes_clear(&attr);
        UA_NodeId_clear(&myIntegerNodeId);
        UA_QualifiedName_clear(&myIntegerName);

        UA_Server_run(mUaServer, &running);
        UA_Server_delete(mUaServer);
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* Create the thread(s) */
  tcpip_init(NULL, NULL);
  struct netif netif_data;

  ip_addr_t ipaddr;
  ip_addr_t netmask;
  ip_addr_t gw;

IP4_ADDR(&ipaddr, 192, 168, 0, 100);
IP4_ADDR(&netmask, 255, 255, 255, 0);
IP4_ADDR(&gw, 192, 168, 0, 1);



  /* add the network interface */
  netif_add(&netif_data, &ipaddr, &netmask, &gw, NULL, NULL, &ethernet_input);

  /*  Registers the default network interface */
  netif_set_default(&netif_data);

  //ethernet_link_status_updated(&netif_data);

#if LWIP_NETIF_LINK_CALLBACK
  netif_set_link_callback(&netif_data, NULL);
#endif

  /* definition and creation of defaultTask */
  xTaskCreate(opcua_thread, "OPCUA Task", 256, NULL, 1, NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* Start scheduler */
vTaskStartScheduler(); 

  /* We should never get here as control is now taken by the scheduler */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

