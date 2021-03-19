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
 * Copyright 2016-2020 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ENET_ETHERNETIF_H
#define ENET_ETHERNETIF_H

#include "lwip/err.h"
#include "lwip/netif.h"
#include "fsl_enet.h"
#include "fsl_phy.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#ifndef ENET_RXBD_NUM
    #define ENET_RXBD_NUM (5)
#endif
#ifndef ENET_TXBD_NUM
#if defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0)
    #define ENET_TXBD_NUM (5)
#else
    #define ENET_TXBD_NUM (3)
#endif
#endif
#ifndef ENET_RXBUFF_SIZE
#if defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0)
    #define ENET_RXBUFF_SIZE (ENET_FRAME_MAX_FRAMELEN + ETH_PAD_SIZE)
#else
    #define ENET_RXBUFF_SIZE ENET_FRAME_MAX_FRAMELEN
#endif
#endif
   
#ifndef ENET_TXBUFF_SIZE
    #define ENET_TXBUFF_SIZE (ENET_FRAME_MAX_FRAMELEN)
#endif

#define ENET_TIMEOUT        (0xFFFU)

/* ENET IRQ priority. Used in FreeRTOS. */
/* Interrupt priorities. */
#ifdef __CA7_REV
#ifndef ENET_PRIORITY
    #define ENET_PRIORITY       (21U)
#endif
#ifndef ENET_1588_PRIORITY
    #define ENET_1588_PRIORITY  (20U)
#endif
#else
#ifndef ENET_PRIORITY
    #define ENET_PRIORITY       (6U)
#endif
#ifndef ENET_1588_PRIORITY
    #define ENET_1588_PRIORITY  (5U)
#endif
#endif

/*  Defines Ethernet Autonegotiation Timeout during initialization. 
 *  Set it to 0 to disable the waiting. */ 
#ifndef ENET_ATONEGOTIATION_TIMEOUT
    #define ENET_ATONEGOTIATION_TIMEOUT     (0xFFFU)
#endif

/* Define those to better describe your network interface. */
#define IFNAME0 'e'
#define IFNAME1 'n'

#if defined(FSL_SDK_ENABLE_DRIVER_CACHE_CONTROL) && FSL_SDK_ENABLE_DRIVER_CACHE_CONTROL
    #if defined(FSL_FEATURE_L2CACHE_LINESIZE_BYTE) \
        && ((!defined(FSL_SDK_DISBLE_L2CACHE_PRESENT)) || (FSL_SDK_DISBLE_L2CACHE_PRESENT == 0))
        #if defined(FSL_FEATURE_L1DCACHE_LINESIZE_BYTE)
            #define FSL_CACHE_LINESIZE_MAX MAX(FSL_FEATURE_L1DCACHE_LINESIZE_BYTE, FSL_FEATURE_L2CACHE_LINESIZE_BYTE)
            #define FSL_ENET_BUFF_ALIGNMENT MAX(ENET_BUFF_ALIGNMENT, FSL_CACHE_LINESIZE_MAX)
        #else
            #define FSL_ENET_BUFF_ALIGNMENT MAX(ENET_BUFF_ALIGNMENT, FSL_FEATURE_L2CACHE_LINESIZE_BYTE)
        #endif
    #elif defined(FSL_FEATURE_L1DCACHE_LINESIZE_BYTE)
        #define FSL_ENET_BUFF_ALIGNMENT MAX(ENET_BUFF_ALIGNMENT, FSL_FEATURE_L1DCACHE_LINESIZE_BYTE)
    #else
        #define FSL_ENET_BUFF_ALIGNMENT ENET_BUFF_ALIGNMENT
    #endif
#else
    #define FSL_ENET_BUFF_ALIGNMENT ENET_BUFF_ALIGNMENT
#endif

#define ENET_RING_NUM 1U

typedef uint8_t rx_buffer_t[SDK_SIZEALIGN(ENET_RXBUFF_SIZE, FSL_ENET_BUFF_ALIGNMENT)];
typedef uint8_t tx_buffer_t[SDK_SIZEALIGN(ENET_TXBUFF_SIZE, FSL_ENET_BUFF_ALIGNMENT)];

#if (defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0))
typedef struct mem_range
{
    uint32_t start;
    uint32_t end;
} mem_range_t;
#endif /* FSL_FEATURE_SOC_LPC_ENET_COUNT */

/**
 * Helper struct to hold data for configuration of ethernet interface.
 */
typedef struct ethernetif_config
{
    phy_handle_t *phyHandle;
    uint8_t macAddress[NETIF_MAX_HWADDR_LEN];
#if (defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0))
    const mem_range_t *non_dma_memory;
#endif /* FSL_FEATURE_SOC_LPC_ENET_COUNT */
} ethernetif_config_t;

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

/**
 * This function should be passed as a parameter to netif_add()
 * if you initialize the first ENET interface.
 */
err_t ethernetif0_init(struct netif *netif);

#if (defined(FSL_FEATURE_SOC_ENET_COUNT) && (FSL_FEATURE_SOC_ENET_COUNT > 1)) \
 || (defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 1))
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
 * @param netif the lwip network interface structure for this ethernetif
 */
void ethernetif_input( struct netif *netif);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* ENET_ETHERNETIF_H */
