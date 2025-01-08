/*
 * Copyright (c) 2001-2003 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * Copyright (c) 2013-2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ETHERNETIF_H
#define ETHERNETIF_H

#include <lwip/err.h>
#include <lwip/netif.h>
#include <fsl_phy.h>
#include <lwipopts.h>
#if defined(FSL_FEATURE_SOC_RGPIO_COUNT) && (FSL_FEATURE_SOC_RGPIO_COUNT > 0)
#include <fsl_rgpio.h>
#else
#include <fsl_gpio.h>
#endif

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* Disable Rx interrupt when out of RX buffers. Enable when first buffer is freed. */
#ifndef ETH_DISABLE_RX_INT_WHEN_OUT_OF_BUFFERS
#define ETH_DISABLE_RX_INT_WHEN_OUT_OF_BUFFERS (!NO_SYS)
#endif

#define ETHERNETIF_TIMEOUT (0xFFFU)

#ifndef ETH_LINK_POLLING_INTERVAL_MS
#define ETH_LINK_POLLING_INTERVAL_MS (1500)
#endif

/* ENET IRQ priority. Used in FreeRTOS. */
/* Interrupt priorities. */ // FIXME rename or move?
#ifdef __CA7_REV
#ifndef ENET_PRIORITY
#define ENET_PRIORITY (21U)
#endif
#ifndef ENET_1588_PRIORITY
#define ENET_1588_PRIORITY (20U)
#endif
#else
#ifndef ENET_PRIORITY
#define ENET_PRIORITY (6U)
#endif
#ifndef ENET_1588_PRIORITY
#define ENET_1588_PRIORITY (5U)
#endif
#endif

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

#define ETHERNETIF_RING_NUM 1U

#define ETHERNETIF_WAIT_FOREVER (0U)

/**
 * Helper struct to hold data for configuration of ethernet interface.
 */
typedef struct ethernetif_config
{
    phy_handle_t *phyHandle;
    uint8_t phyAddr;
    const phy_operations_t *phyOps;
    void *phyResource;
    uint32_t srcClockHz;
    uint8_t macAddress[NETIF_MAX_HWADDR_LEN];

#if NO_SYS == 0
#if defined(FSL_FEATURE_SOC_RGPIO_COUNT) && (FSL_FEATURE_SOC_RGPIO_COUNT > 0)
    RGPIO_Type *phyIntGpio;
#else
    GPIO_Type *phyIntGpio;
#endif
    uint8_t phyIntGpioPin;
#endif
} ethernetif_config_t;

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/**
 * This function should be passed as a parameter to netif_add()
 * if you initialize the first ENET interface.
 */
err_t ethernetif0_init(struct netif *netif);

#if (defined(FSL_FEATURE_SOC_ENET_COUNT) && (FSL_FEATURE_SOC_ENET_COUNT > 1)) || \
    (defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 1))
/**
 * This function should be passed as a parameter to netif_add()
 * if you initialize the second ENET interface.
 */
err_t ethernetif1_init(struct netif *netif);
#endif /* FSL_FEATURE_SOC_*_ENET_COUNT */

/**
 * This function should be called when a packet is ready to be read
 * from the interface.
 * It is used by bare-metal applications.
 *
 * @param netif_ the lwip network interface structure for this ethernetif
 */
void ethernetif_input(struct netif *netif_);

/**
 * This function probes phy for current link state, speed and duplex and
 * passes those information to lwIP and ethernet driver.
 *
 * This function is automatically called every @ref ETH_LINK_POLLING_INTERVAL_MS.
 * Define this macro to zero (in lwipopts.h or globally) to disable polling.
 *
 * Function can be called manually but in case of FreeRTOS, lwIP core
 * must be locked or the call has to be executed from tcpip_thread.
 *
 * @param netif_ the lwip network interface structure for this ethernetif
 */
void ethernetif_probe_link(struct netif *netif_);

#if (defined(SDK_OS_FREE_RTOS) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1)) || (!defined(SDK_OS_FREE_RTOS))
/**
 * This function blocks until the link on selected netif becomes up.
 *
 * If RTOS is used and LWIP_NETIF_EXT_STATUS_CALLBACK is defined to 1 thread
 * will be blocked on queue. On baremetal, busy wait will be used.
 *
 * @warn The function is not reentrant and also must NOT be called concurently
 *       with @ref ethernetif_wait_ipv4_valid.
 *
 * @param netif_       Selected interface
 * @param timeout_ms   Maximum time to wait in ms or @ref ETHERNETIF_WAIT_FOREVER
 *
 * @return  ERR_OK       When interface becomes up
 *          ERR_TIMEOUT  Timeout reached
 *          ERR_MEM      Out of memory, can't allocate queue
 */
err_enum_t ethernetif_wait_linkup(struct netif *netif_, long timeout_ms);

/**
 * This function blocks until the link on at least one netif in array becomes up.
 *
 * If RTOS is used and LWIP_NETIF_EXT_STATUS_CALLBACK is defined to 1 thread
 * will be blocked on queue. On baremetal, busy wait will be used.
 *
 * @warn The function is not reentrant and also must NOT be called concurently
 *       with @ref ethernetif_wait_ipv4_valid.
 *
 * @param netif_array       Array of selected interfaces
 * @param netif_array_len   Length of netif_array
 * @param timeout_ms        Maximum time to wait in ms or @ref ETHERNETIF_WAIT_FOREVER
 *
 * @return  ERR_OK       When interface becomes up
 *          ERR_TIMEOUT  Timeout reached
 *          ERR_MEM      Out of memory, can't allocate queue
 */
err_enum_t ethernetif_wait_linkup_array(struct netif **netif_array, int netif_array_len, long timeout_ms);

#if LWIP_DHCP == 1
/**
 * This function blocks until IPv4 addres on netif becomes valid (diferrent from
 * IPADDR_ANY 255.255.255.255).
 *
 * Please set IP addres to IPADDR_ANY before call of this function for example
 * by call of @ref netif_add_noaddr.
 *
 * If RTOS is used and LWIP_NETIF_EXT_STATUS_CALLBACK is defined to 1 thread
 * will be blocked on queue. On baremetal, busy wait will be used.
 *
 * @warn The function is not reentrant and also must NOT be called concurently
 *       with @ref ethernetif_wait_linkup.
 *
 * @param netif_       Selected interface
 * @param timeout_ms   Maximum time to wait in ms or @ref ETHERNETIF_WAIT_FOREVER
 *
 * @return  ERR_OK       When IPv4 address becomes valid
 *          ERR_TIMEOUT  Timeout reached
 *          ERR_MEM      Out of memory, can't allocate queue
 */
err_enum_t ethernetif_wait_ipv4_valid(struct netif *netif_, long timeout_ms);
#endif /* LWIP_DHCP == 1 */

#endif /* #if (defined(SDK_OS_FREE_RTOS) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1)) || \
              (!defined(SDK_OS_FREE_RTOS)) */

/**
 * Returns interface's last known link speed.
 *
 * @param netif_  the lwip network interface
 * @return the speed
 */
phy_speed_t ethernetif_get_link_speed(struct netif *netif_);

/**
 * Returns interface's last known duplex state.
 *
 * @param netif_  the lwip network interface
 * @return the state
 */
phy_duplex_t ethernetif_get_link_duplex(struct netif *netif_);

#if ((LWIP_IPV6 == 1) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1))
/**
 * This function sets a callback to be invoke when any of IPv6 addresses'
 * state changes from valid to not-valid or vice versa.
 *
 * @param callback_fn The callback function to set,
 *                    can be NULL to remove a previously set callback.
 */
void set_ipv6_valid_state_cb(netif_status_callback_fn callback_fn);
#endif /* ((LWIP_IPV6 == 1) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1)) */

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* ENET_ETHERNETIF_H */
