/*
 * Copyright 2019 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef ENET_ETHERNETIF_PRIV_H
#define ENET_ETHERNETIF_PRIV_H

#include "lwip/err.h"

struct ethernetif;

#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

err_t ethernetif_init(struct netif *netif, struct ethernetif *ethernetif,
                      const uint8_t enetIdx,
                      const ethernetif_config_t *ethernetifConfig);

void ethernetif_enet_init(struct netif *netif, struct ethernetif *ethernetif,
                          const ethernetif_config_t *ethernetifConfig);

void ethernetif_phy_init(struct ethernetif *ethernetif,
                         const ethernetif_config_t *ethernetifConfig,
                         enet_config_t *config);

ENET_Type **ethernetif_enet_ptr(struct ethernetif *ethernetif);

#if LWIP_IPV4 && LWIP_IGMP
err_t ethernetif_igmp_mac_filter(struct netif *netif, const ip4_addr_t *group,
                                 enum netif_mac_filter_action action);
#endif

#if LWIP_IPV6 && LWIP_IPV6_MLD
err_t ethernetif_mld_mac_filter(struct netif *netif, const ip6_addr_t *group,
                                enum netif_mac_filter_action action);
#endif

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
struct pbuf *ethernetif_linkinput(struct netif *netif);

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
err_t ethernetif_linkoutput(struct netif *netif, struct pbuf *p);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

#endif /* ENET_ETHERNETIF_PRIV_H */
