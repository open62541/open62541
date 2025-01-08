/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
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
 * Copyright 2016-2020,2022-2024 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <fsl_common.h>

#include <lwip/opt.h>
#include <lwip/def.h>
#include <lwip/mem.h>
#include <lwip/pbuf.h>
#include <lwip/stats.h>
#include <lwip/snmp.h>
#include <lwip/ethip6.h>
#include <netif/etharp.h>
#include <netif/ppp/pppoe.h>
#include <lwip/igmp.h>
#include <lwip/mld6.h>
#include <lwip/timeouts.h>
#include <lwip/netif.h>
#include <lwip/sys.h>

#if USE_RTOS && defined(SDK_OS_FREE_RTOS)
#include <FreeRTOS.h>
#include <event_groups.h>
#include <portmacro.h>
#include <lwip/tcpip.h>
#endif

#include <ethernetif_priv.h>

#include "ethernetif.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#if ETH_LINK_POLLING_INTERVAL_MS > 0
static void probe_link_cyclic(void *netif_);
#endif

#if defined(SDK_OS_FREE_RTOS) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1)

#ifndef ETH_WAIT_QUEUE_MSG_CNT
#define ETH_WAIT_QUEUE_MSG_CNT (1)
#endif

struct extcb_msg
{
    struct netif *cb_netif;
    netif_nsc_reason_t reason;
};

static QueueHandle_t extcb_queue = NULL;

#endif /* defined(SDK_OS_FREE_RTOS) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1) */

/* Under FreeRTOS, define ETH_DO_RX_IN_SEPARATE_TASK 0 to move processing of received
 * packets from a separate task to an interrupt. It increases throughput and saves some
 * memory from a separate task stack at cost of worse system response time.
 */
#ifndef SDK_OS_FREE_RTOS
#define ETH_DO_RX_IN_SEPARATE_TASK 0
#endif

#ifndef ETH_DO_RX_IN_SEPARATE_TASK
#define ETH_DO_RX_IN_SEPARATE_TASK 1
#endif

#if ETH_DO_RX_IN_SEPARATE_TASK

#ifndef ETH_RX_TASK_STACK_SIZE
#define ETH_RX_TASK_STACK_SIZE (512)
#endif

#ifndef ETH_RX_TASK_PRIO
#define ETH_RX_TASK_PRIO ((configTIMER_TASK_PRIORITY)-1)
#endif

static TaskHandle_t ethernetif_rx_task = NULL;

#endif /* #if ETH_DO_RX_IN_SEPARATE_TASK */

#if ((LWIP_IPV6 == 1) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1))
static netif_status_callback_fn ipv6_valid_state_user_cb;
#endif /* ((LWIP_IPV6 == 1) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1)) */

/*******************************************************************************
 * Code
 ******************************************************************************/

#if ETH_LINK_POLLING_INTERVAL_MS == 0 && NO_SYS == 0
static void phy_irq_synced_handler(void *arg)
{
    struct netif *netif_ = (struct netif *)arg;
    ethernetif_probe_link(netif_);
}

static void ethernetif_phy_irq_callback(void *arg)
{
    struct netif *netif_ = (struct netif *)arg;

    hal_gpio_handle_t gpioHdl = ethernetif_get_int_gpio_hdl(netif_);
    assert(gpioHdl != NULL);

    phy_handle_t *phy = ethernetif_get_phy(netif_);
    assert(phy != NULL);

    uint8_t state = 0;
    if (HAL_GpioGetInput(gpioHdl, &state) != kStatus_HAL_GpioSuccess)
    {
        PHY_ClearInterrupt(phy);
        return;
    }

    // Spurious interrupt
    if (state != 0)
    {
        PHY_ClearInterrupt(phy);
        return;
    }

    PHY_ClearInterrupt(phy);
    tcpip_try_callback(phy_irq_synced_handler, netif_);
}

#if defined(FSL_FEATURE_SOC_RGPIO_COUNT) && (FSL_FEATURE_SOC_RGPIO_COUNT > 0)
static uint8_t ethernetif_gpio_base_to_idx(RGPIO_Type *base)
{
    RGPIO_Type *types[] = RGPIO_BASE_PTRS;
#else
static uint8_t ethernetif_gpio_base_to_idx(GPIO_Type *base)
{
    GPIO_Type *types[] = GPIO_BASE_PTRS;
#endif

    for (size_t i = 0; i < ARRAY_SIZE(types); i++)
    {
        if (types[i] == base)
            return i;
    }

    return 0;
}
#endif

void ethernetif_phy_init(struct ethernetif *ethernetif, const ethernetif_config_t *ethernetifConfig)
{
    status_t status;
    phy_config_t phyConfig = {
        .phyAddr  = ethernetifConfig->phyAddr,
        .ops      = ethernetifConfig->phyOps,
        .resource = ethernetifConfig->phyResource,
        .autoNeg  = true,
    };

    LWIP_PLATFORM_DIAG(("Initializing PHY...\r\n"));

    status = PHY_Init(ethernetifConfig->phyHandle, &phyConfig);

    if (kStatus_Success != status)
    {
        LWIP_ASSERT("\r\nCannot initialize PHY.\r\n", 0);
    }
}

static void fetch_received_pkts(struct netif *netif_)
{
    /* Move received packets into new pbufs. */

#if !defined(ETH_MAX_RX_PKTS_AT_ONCE) || (ETH_MAX_RX_PKTS_AT_ONCE < 1) || !NO_SYS
    while (1)
#else
    u16_t n;
    for (n = 0; n < (ETH_MAX_RX_PKTS_AT_ONCE); n++)
#endif
    {
        struct pbuf *p = ethernetif_linkinput(netif_);
        if (p == NULL)
        {
            /* No more received packets available. */
            break;
        }

        /* Pass all packets to ethernet_input, which decides what packets it supports */
        if (netif_->input(p, netif_) != (err_t)ERR_OK)
        {
            LWIP_DEBUGF(NETIF_DEBUG, ("fetch_received_pkts: IP input error\n"));
            ethernetif_pbuf_free_safe(p);
            p = NULL;
        }
    }
}

#if ETH_DO_RX_IN_SEPARATE_TASK
static uint32_t netif_to_bitmask(const struct netif *netif_)
{
    const uint32_t mask = 1U << (netif_get_index(netif_) % 32);
    return mask;
}

static void rx_task(void *arg)
{
    (void)arg;
    struct netif *netif_;

    while (true)
    {
        uint32_t bits = 0;
        xTaskNotifyWait(0U /*ulBitsToClearOnEntry*/, 0xFFFFFFFFU /*ulBitsToClearOnExit*/, &bits, portMAX_DELAY);

        /* When notified form ISR fetch all received packet from desired netifs. */
        NETIF_FOREACH(netif_)
        {
            if (0U != (bits & netif_to_bitmask(netif_)))
            {
                fetch_received_pkts(netif_);
            }
        }
    }
}
#endif /* ETH_DO_RX_IN_SEPARATE_TASK */

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function ethernetif_linkinput() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif_ the lwip network interface structure for this ethernetif
 */
void ethernetif_input(struct netif *netif_)
{
#if ETH_DO_RX_IN_SEPARATE_TASK
    (void)netif_;

#ifdef __CA7_REV
    if (SystemGetIRQNestingLevel())
#else /* __CA7_REV */
    if (__get_IPSR())
#endif
    {
        portBASE_TYPE taskToWake = pdFALSE;
        xTaskNotifyFromISR(ethernetif_rx_task, netif_to_bitmask(netif_), eSetBits, &taskToWake);
        if (taskToWake == pdTRUE)
        {
            portYIELD_FROM_ISR(taskToWake);
        }
    }
    else
    {
        (void)xTaskNotifyGive(ethernetif_rx_task);
    }
#else
    fetch_received_pkts(netif_);
#endif /* ETH_DO_RX_IN_SEPARATE_TASK */
}

err_t ethernetif_init(struct netif *netif_,
                      struct ethernetif *ethernetif,
                      void *drvBase,
                      const ethernetif_config_t *ethernetifConfig)
{
    LWIP_ASSERT("netif_ != NULL", (netif_ != NULL));
    LWIP_ASSERT("ethernetifConfig != NULL", (ethernetifConfig != NULL));

#if LWIP_NETIF_HOSTNAME
    /* Initialize interface hostname */
    netif_->hostname = "lwip";
#endif /* LWIP_NETIF_HOSTNAME */

#if ETH_DO_RX_IN_SEPARATE_TASK
    /* We have only 32 bits (one bit for each netif) in task notification
       but app could create up to 254 netifs. On each remove -> add index
       is rising and then wrapped around 255. */
    LWIP_ASSERT("Too many netif creations.", netif_get_index(netif_) <= 32);

    /* Initialize lwIP rx thread */
    if (NULL == ethernetif_rx_task)
    {
        BaseType_t ret =
            xTaskCreate(rx_task, "lwip_rx", ETH_RX_TASK_STACK_SIZE, NULL, ETH_RX_TASK_PRIO, &ethernetif_rx_task);
        if (ret != pdPASS)
        {
            ethernetif_rx_task = NULL;
            LWIP_ASSERT("ethernetif_init(): RX task creation failed.", 0);
            return ERR_MEM;
        }
    }
#endif /* ETH_DO_RX_IN_SEPARATE_TASK */

    /*
     * Initialize the snmp variables and counters inside the struct netif.
     * The last argument should be replaced with your link speed, in units
     * of bits per second.
     */
    MIB2_INIT_NETIF(netif_, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

    netif_->state   = ethernetif;
    netif_->name[0] = IFNAME0;
    netif_->name[1] = IFNAME1;
/* We directly use etharp_output() here to save a function call.
 * You can instead declare your own function an call etharp_output()
 * from it if you have to do some checks before sending (e.g. if link
 * is available...) */
#if LWIP_IPV4
    netif_->output = etharp_output;
#endif
#if LWIP_IPV6
    netif_->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
    netif_->linkoutput = ethernetif_linkoutput;

#if LWIP_IPV4 && LWIP_IGMP
    netif_set_igmp_mac_filter(netif_, ethernetif_igmp_mac_filter);
    netif_->flags |= NETIF_FLAG_IGMP;
#endif
#if LWIP_IPV6 && LWIP_IPV6_MLD
    netif_set_mld_mac_filter(netif_, ethernetif_mld_mac_filter);
    netif_->flags |= NETIF_FLAG_MLD6;
#endif

    /* Init ethernetif parameters.*/
    *ethernetif_base_ptr(ethernetif) = drvBase;
    LWIP_ASSERT("*ethernetif_base_ptr(ethernetif) != NULL", (*ethernetif_base_ptr(ethernetif) != NULL));

    /* set MAC hardware address length */
    netif_->hwaddr_len = ETH_HWADDR_LEN;

    /* set MAC hardware address */
    memcpy(netif_->hwaddr, ethernetifConfig->macAddress, NETIF_MAX_HWADDR_LEN);

    /* maximum transfer unit */
    netif_->mtu = 1500; /* TODO: define a config */

    /* device capabilities */
    /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
    netif_->flags |= NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_LINK_UP;

    /* Driver initialization.*/
    ethernetif_plat_init(netif_, ethernetif, ethernetifConfig);

    /* Start polling link state */
#if ETH_LINK_POLLING_INTERVAL_MS > 0
    probe_link_cyclic(netif_);
#elif NO_SYS == 0
    if (ethernetifConfig->phyIntGpio != NULL)
    {
        hal_gpio_handle_t gpioHdl = ethernetif_get_int_gpio_hdl(netif_);
        assert(gpioHdl != NULL);

        hal_gpio_pin_config_t cfg = {.direction = kHAL_GpioDirectionIn,
                                     .port      = ethernetif_gpio_base_to_idx(ethernetifConfig->phyIntGpio),
                                     .pin       = ethernetifConfig->phyIntGpioPin};

        if (HAL_GpioInit(gpioHdl, &cfg) != kStatus_HAL_GpioSuccess)
            return ERR_VAL;

        if (HAL_GpioInstallCallback(gpioHdl, ethernetif_phy_irq_callback, netif_) != kStatus_HAL_GpioSuccess)
            return ERR_VAL;

        if (HAL_GpioSetTriggerMode(gpioHdl, kHAL_GpioInterruptFallingEdge) != kStatus_HAL_GpioSuccess)
            return ERR_VAL;

        phy_handle_t *phy = ethernetif_get_phy(netif_);
        LWIP_ASSERT("PHY component does not implement link interrupt", phy->ops->enableLinkInterrupt != NULL);
        PHY_EnableLinkInterrupt(phy, kPHY_IntrActiveLow);

        ethernetif_probe_link(netif_);
    }
#endif

#if LWIP_IPV6 && LWIP_IPV6_MLD
    /*
     * For hardware/netifs that implement MAC filtering.
     * All-nodes link-local is handled by default, so we must let the hardware know
     * to allow multicast packets in.
     * Should set mld_mac_filter previously. */
    if (netif_->mld_mac_filter != NULL)
    {
        ip6_addr_t ip6_allnodes_ll;
        ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
        netif_->mld_mac_filter(netif_, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
    }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

    return ERR_OK;
}

void ethernetif_probe_link(struct netif *netif_)
{
    phy_handle_t *phy = ethernetif_get_phy(netif_);
    bool status; // true = Link is up

    status_t st = PHY_GetLinkStatus(phy, &status);

    if (st == kStatus_Success)
    {
        LWIP_ASSERT_CORE_LOCKED();
        if (status)
        {
            phy_speed_t speed;
            phy_duplex_t duplex;

            st = PHY_GetLinkSpeedDuplex(phy, &speed, &duplex);
            if (st == kStatus_Success)
            {
                ethernetif_on_link_up(netif_, speed, duplex);
            }
            netif_set_link_up(netif_);
        }
        else
        {
            netif_set_link_down(netif_);
            ethernetif_on_link_down(netif_);
        }
    }
}

#if ETH_LINK_POLLING_INTERVAL_MS > 0
static void probe_link_cyclic(void *netif_)
{
    sys_timeout(ETH_LINK_POLLING_INTERVAL_MS, probe_link_cyclic, netif_);

    ethernetif_probe_link(netif_);
}
#endif /* ETH_LINK_POLLING_INTERVAL_MS > 0 */

#if defined(SDK_OS_FREE_RTOS) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1)

static void extcb(struct netif *cb_netif, netif_nsc_reason_t reason, const netif_ext_callback_args_t *args)
{
    (void)args;

    struct extcb_msg msg = {cb_netif, reason};
    (void)xQueueSend(extcb_queue, (void *)&msg, portMAX_DELAY);
}

#endif /* #if defined(SDK_OS_FREE_RTOS) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1) */

#if (defined(SDK_OS_FREE_RTOS) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1)) || (!defined(SDK_OS_FREE_RTOS))

err_enum_t ethernetif_wait_linkup(struct netif *netif_, long timeout_ms)
{
    return ethernetif_wait_linkup_array(&netif_, 1, timeout_ms);
}

err_enum_t ethernetif_wait_linkup_array(struct netif **netif_array, int netif_array_len, long timeout_ms)
{
#ifdef SDK_OS_FREE_RTOS

    TickType_t timeout_ticks =
        (ETHERNETIF_WAIT_FOREVER == timeout_ms) ? portMAX_DELAY : timeout_ms / portTICK_PERIOD_MS;
    netif_ext_callback_t cb_mem;
    err_enum_t ret = ERR_INPROGRESS;

    LWIP_ASSERT("Timeout is below tick resolution.", timeout_ticks >= 1);
    LWIP_ASSERT("This function must not be called concurrently.", NULL == extcb_queue);

    /* Be fast if condition is already met */
    for (int i = 0; i < netif_array_len; i++)
    {
        if (netif_is_link_up(netif_array[i]))
        {
            return ERR_OK;
        }
    }

    extcb_queue = xQueueCreate(ETH_WAIT_QUEUE_MSG_CNT, sizeof(struct extcb_msg));

    if (NULL == extcb_queue)
    {
        LWIP_ASSERT("Can't allocate queue", false);
        return ERR_MEM;
    }

    LOCK_TCPIP_CORE();
    netif_add_ext_callback(&cb_mem, extcb);
    UNLOCK_TCPIP_CORE();

    /* Event might occur before CB was registered, so check it to prevent eternal wait for message. */
    bool netif_is_up = false;

    for (int i = 0; i < netif_array_len; i++)
    {
        if (netif_is_link_up(netif_array[i]))
        {
            netif_is_up = true;
            break;
        }
    }

    if (!netif_is_up)
    {
        do
        {
            struct extcb_msg msg;

            if (xQueueReceive(extcb_queue, &msg, timeout_ticks) == pdPASS)
            {
                for (int i = 0; i < netif_array_len; i++)
                {
                    if ((msg.cb_netif == netif_array[i]) && (msg.reason & LWIP_NSC_LINK_CHANGED))
                    {
                        if (netif_is_link_up(netif_array[i]))
                        {
                            ret = ERR_OK;
                        }
                    }
                }
            }
            else
            {
                ret = ERR_TIMEOUT;
            }
        } while (ret == ERR_INPROGRESS);
    }

    LOCK_TCPIP_CORE();
    netif_remove_ext_callback(&cb_mem);
    UNLOCK_TCPIP_CORE();
    if (NULL != extcb_queue)
    {
        vQueueDelete(extcb_queue);
        extcb_queue = NULL;
    }

    return ret;

#else  /* no RTOS */

    uint32_t now                = sys_now();
    const uint32_t force_wrap   = now >= (UINT32_MAX / 2) ? (UINT32_MAX / 2) : 0U;
    const uint32_t timeout_time = (now + force_wrap) + timeout_ms;

    LWIP_ASSERT("Timeout too long.", timeout_ms <= (UINT32_MAX / 2));

    while (true)
    {
        /* PHY polling is done in check timeouts so do it during wait. */
        sys_check_timeouts();

        if (ETHERNETIF_WAIT_FOREVER != timeout_ms)
        {
            now = sys_now() + force_wrap;

            if ((now >= timeout_time))
            {
                return ERR_TIMEOUT;
            }
        }

        for (int i = 0; i < netif_array_len; i++)
        {
            if (netif_is_link_up(netif_array[i]))
            {
                return ERR_OK;
            }
        }
    }

#endif /* defined(SDK_OS_FREE_RTOS) */
}

#if LWIP_DHCP == 1
err_enum_t ethernetif_wait_ipv4_valid(struct netif *netif_, long timeout_ms)
{
#ifdef SDK_OS_FREE_RTOS
    TickType_t timeout_ticks =
        (ETHERNETIF_WAIT_FOREVER == timeout_ms) ? portMAX_DELAY : timeout_ms / portTICK_PERIOD_MS;
    netif_ext_callback_t cb_mem;
    err_enum_t ret = ERR_INPROGRESS;

    LWIP_ASSERT("Timeout is below tick resolution.", timeout_ticks >= 1);
    LWIP_ASSERT("This function must not be called concurrently.", NULL == extcb_queue);

    /* Be fast if we already have address */
    if (!ip4_addr_isany_val(*netif_ip4_addr(netif_)))
    {
        return ERR_OK;
    }

    extcb_queue = xQueueCreate(ETH_WAIT_QUEUE_MSG_CNT, sizeof(struct extcb_msg));

    if (NULL == extcb_queue)
    {
        LWIP_ASSERT("Can't allocate queue", false);
        return ERR_MEM;
    }

    LOCK_TCPIP_CORE();
    netif_add_ext_callback(&cb_mem, extcb);
    UNLOCK_TCPIP_CORE();

    /* Event might occur before CB was registered, so check it to prevent eternal wait for message. */
    if (ip4_addr_isany_val(*netif_ip4_addr(netif_)))
    {
        do
        {
            struct extcb_msg msg;

            if (xQueueReceive(extcb_queue, &msg, timeout_ticks) == pdPASS)
            {
                if ((msg.cb_netif == netif_) && (msg.reason & LWIP_NSC_IPV4_ADDR_VALID))
                {
                    ret = ERR_OK;
                }
            }
            else
            {
                // timeout
                ret = ERR_TIMEOUT;
            }
        } while (ret == ERR_INPROGRESS);
    }

    LOCK_TCPIP_CORE();
    netif_remove_ext_callback(&cb_mem);
    UNLOCK_TCPIP_CORE();
    if (NULL != extcb_queue)
    {
        vQueueDelete(extcb_queue);
        extcb_queue = NULL;
    }

    return ret;

#else  /* no RTOS */

    uint32_t now                = sys_now();
    const uint32_t force_wrap   = now >= (UINT32_MAX / 2) ? (UINT32_MAX / 2) : 0U;
    const uint32_t timeout_time = (now + force_wrap) + timeout_ms;

    LWIP_ASSERT("Timeout too long.", timeout_ms <= (UINT32_MAX / 2));

    do
    {
        /* Read packets and let lwIP work to do DHCP. */
        ethernetif_input(netif_);
        sys_check_timeouts();

        if (ETHERNETIF_WAIT_FOREVER != timeout_ms)
        {
            now = sys_now() + force_wrap;

            if ((now >= timeout_time))
            {
                return ERR_TIMEOUT;
            }
        }

    } while (ip4_addr_isany_val(*netif_ip4_addr(netif_)));

    return ERR_OK;

#endif /* USE_RTOS && defined(SDK_OS_FREE_RTOS) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1) */
}
#endif /* LWIP_DHCP == 1 */

#endif /*(defined(SDK_OS_FREE_RTOS) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1)) || \
       (!defined(SDK_OS_FREE_RTOS))*/

#if ((LWIP_IPV6 == 1) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1))

static void ipv6_valid_state_cb(struct netif *netif_, netif_nsc_reason_t reason, const netif_ext_callback_args_t *args)
{
    u8_t new_state;

    if ((reason & LWIP_NSC_IPV6_SET) != 0)
    {
        /*
         * Previous state of the changed address is not known,
         * so just fire the callback to be sure event (if any) is not missed
         */
        ipv6_valid_state_user_cb(netif_);
    }
    else if ((reason & LWIP_NSC_IPV6_ADDR_STATE_CHANGED) != 0)
    {
        new_state = netif_ip6_addr_state(netif_, args->ipv6_addr_state_changed.addr_index);

        if ((new_state & IP6_ADDR_VALID) != (args->ipv6_addr_state_changed.old_state & IP6_ADDR_VALID))
        {
            ipv6_valid_state_user_cb(netif_);
        }
    }
}

void set_ipv6_valid_state_cb(netif_status_callback_fn callback_fn)
{
    static netif_ext_callback_t cb_mem;

    LWIP_ASSERT_CORE_LOCKED();

    if (ipv6_valid_state_user_cb != NULL)
    {
        netif_remove_ext_callback(&cb_mem);
        ipv6_valid_state_user_cb = NULL;
    }

    if (callback_fn != NULL)
    {
        ipv6_valid_state_user_cb = callback_fn;
        netif_add_ext_callback(&cb_mem, ipv6_valid_state_cb);
    }
}
#endif /* ((LWIP_IPV6 == 1) && (LWIP_NETIF_EXT_STATUS_CALLBACK == 1)) */

void ethernetif_pbuf_free_safe(struct pbuf *p)
{
#if NO_SYS
    /* bare metal */
#ifdef __CA7_REV
    if (SystemGetIRQNestingLevel())
#else /* __CA7_REV */
    if (__get_IPSR())
#endif
    {
        /*
         * Inside ISR and pbuf_free_callback is not available in bare metal,
         * so need to assert if memory free from other context is enabled.
         */
        LWIP_ASSERT("Set LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT", LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT);
    }
    pbuf_free(p);
#else
    err_t err;

    /*
     * OS, try to schedule the pbuf_free on tcpip_thread, no matter
     * if LWIP_ALLOW_MEM_FREE_FROM_OTHER_CONTEXT is enabled or not.
     */
    do
    {
        err = pbuf_free_callback(p);
        if (err != ERR_OK)
        {
#ifdef __CA7_REV
            if (SystemGetIRQNestingLevel())
#else  /* __CA7_REV */
            if (__get_IPSR())
#endif
            {
                portYIELD_FROM_ISR(pdTRUE);
            }
            else
            {
                portYIELD();
            }
        }
    } while (err != ERR_OK);
#endif /* NO_SYS */
}
