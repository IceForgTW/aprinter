/**
 * @file
 * Statistics API (to be used from TCPIP thread)
 */

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
#ifndef LWIP_HDR_STATS_H
#define LWIP_HDR_STATS_H

#include "lwip/opt.h"

#include "lwip/mem.h"
#include "lwip/memp.h"

#ifdef __cplusplus
extern "C" {
#endif

#if LWIP_STATS

#ifndef LWIP_STATS_LARGE
#define LWIP_STATS_LARGE 0
#endif

#if LWIP_STATS_LARGE
#define STAT_COUNTER     u32_t
#define STAT_COUNTER_F   U32_F
#else
#define STAT_COUNTER     u16_t
#define STAT_COUNTER_F   U16_F
#endif

struct stats_proto {
  STAT_COUNTER xmit;             /* Transmitted packets. */
  STAT_COUNTER recv;             /* Received packets. */
  STAT_COUNTER fw;               /* Forwarded packets. */
  STAT_COUNTER drop;             /* Dropped packets. */
  STAT_COUNTER chkerr;           /* Checksum error. */
  STAT_COUNTER lenerr;           /* Invalid length error. */
  STAT_COUNTER memerr;           /* Out of memory error. */
  STAT_COUNTER rterr;            /* Routing error. */
  STAT_COUNTER proterr;          /* Protocol error. */
  STAT_COUNTER opterr;           /* Error in options. */
  STAT_COUNTER err;              /* Misc error. */
  STAT_COUNTER cachehit;
};

struct stats_igmp {
  STAT_COUNTER xmit;             /* Transmitted packets. */
  STAT_COUNTER recv;             /* Received packets. */
  STAT_COUNTER drop;             /* Dropped packets. */
  STAT_COUNTER chkerr;           /* Checksum error. */
  STAT_COUNTER lenerr;           /* Invalid length error. */
  STAT_COUNTER memerr;           /* Out of memory error. */
  STAT_COUNTER proterr;          /* Protocol error. */
  STAT_COUNTER rx_v1;            /* Received v1 frames. */
  STAT_COUNTER rx_group;         /* Received group-specific queries. */
  STAT_COUNTER rx_general;       /* Received general queries. */
  STAT_COUNTER rx_report;        /* Received reports. */
  STAT_COUNTER tx_join;          /* Sent joins. */
  STAT_COUNTER tx_leave;         /* Sent leaves. */
  STAT_COUNTER tx_report;        /* Sent reports. */
};

struct stats_mem {
#ifdef LWIP_DEBUG
  const char *name;
#endif /* LWIP_DEBUG */
  STAT_COUNTER err;
  mem_size_t avail;
  mem_size_t used;
  mem_size_t max;
  STAT_COUNTER illegal;
};

struct stats_mib2 {
  /* IP */
  u32_t ipinhdrerrors;
  u32_t ipinaddrerrors;
  u32_t ipinunknownprotos;
  u32_t ipindiscards;
  u32_t ipindelivers;
  u32_t ipoutrequests;
  u32_t ipoutdiscards;
  u32_t ipoutnoroutes;
  u32_t ipreasmoks;
  u32_t ipreasmfails;
  u32_t ipfragoks;
  u32_t ipfragfails;
  u32_t ipfragcreates;
  u32_t ipreasmreqds;
  u32_t ipforwdatagrams;
  u32_t ipinreceives;

  /* TCP */
  u32_t tcpactiveopens;
  u32_t tcppassiveopens;
  u32_t tcpattemptfails;
  u32_t tcpestabresets;
  u32_t tcpoutsegs;
  u32_t tcpretranssegs;
  u32_t tcpinsegs;
  u32_t tcpinerrs;
  u32_t tcpoutrsts;

  /* UDP */
  u32_t udpindatagrams;
  u32_t udpnoports;
  u32_t udpinerrors;
  u32_t udpoutdatagrams;

  /* ICMP */
  u32_t icmpinmsgs;
  u32_t icmpinerrors;
  u32_t icmpindestunreachs;
  u32_t icmpintimeexcds;
  u32_t icmpinparmprobs;
  u32_t icmpinsrcquenchs;
  u32_t icmpinredirects;
  u32_t icmpinechos;
  u32_t icmpinechoreps;
  u32_t icmpintimestamps;
  u32_t icmpintimestampreps;
  u32_t icmpinaddrmasks;
  u32_t icmpinaddrmaskreps;
  u32_t icmpoutmsgs;
  u32_t icmpouterrors;
  u32_t icmpoutdestunreachs;
  u32_t icmpouttimeexcds;
  u32_t icmpoutechos; /* can be incremented by user application ('ping') */
  u32_t icmpoutechoreps;
};

struct stats_mib2_netif_ctrs {
  u32_t ifinoctets;
  u32_t ifinucastpkts;
  u32_t ifinnucastpkts;
  u32_t ifindiscards;
  u32_t ifinerrors;
  u32_t ifinunknownprotos;
  u32_t ifoutoctets;
  u32_t ifoutucastpkts;
  u32_t ifoutnucastpkts;
  u32_t ifoutdiscards;
  u32_t ifouterrors;
};

struct stats_ {
#if LINK_STATS
  struct stats_proto link;
#endif
#if ETHARP_STATS
  struct stats_proto etharp;
#endif
#if IPFRAG_STATS
  struct stats_proto ip_frag;
#endif
#if IP_STATS
  struct stats_proto ip;
#endif
#if ICMP_STATS
  struct stats_proto icmp;
#endif
#if IGMP_STATS
  struct stats_igmp igmp;
#endif
#if UDP_STATS
  struct stats_proto udp;
#endif
#if TCP_STATS
  struct stats_proto tcp;
#endif
#if MEMP_STATS
  struct stats_mem memp[MEMP_MAX];
#endif
#if IP6_STATS
  struct stats_proto ip6;
#endif
#if ICMP6_STATS
  struct stats_proto icmp6;
#endif
#if IP6_FRAG_STATS
  struct stats_proto ip6_frag;
#endif
#if MLD6_STATS
  struct stats_igmp mld6;
#endif
#if ND6_STATS
  struct stats_proto nd6;
#endif
#if MIB2_STATS
  struct stats_mib2 mib2;
#endif
};

extern struct stats_ lwip_stats;

void stats_init(void);

#define STATS_INC(x) ++lwip_stats.x
#define STATS_DEC(x) --lwip_stats.x
#define STATS_INC_USED(x, y) do { lwip_stats.x.used += y; \
                                if (lwip_stats.x.max < lwip_stats.x.used) { \
                                    lwip_stats.x.max = lwip_stats.x.used; \
                                } \
                             } while(0)
#define STATS_GET(x) lwip_stats.x
#else /* LWIP_STATS */
#define stats_init()
#define STATS_INC(x)
#define STATS_DEC(x)
#define STATS_INC_USED(x)
#endif /* LWIP_STATS */

#if TCP_STATS
#define TCP_STATS_INC(x) STATS_INC(x)
#define TCP_STATS_DISPLAY() stats_display_proto(&lwip_stats.tcp, "TCP")
#else
#define TCP_STATS_INC(x)
#define TCP_STATS_DISPLAY()
#endif

#if UDP_STATS
#define UDP_STATS_INC(x) STATS_INC(x)
#define UDP_STATS_DISPLAY() stats_display_proto(&lwip_stats.udp, "UDP")
#else
#define UDP_STATS_INC(x)
#define UDP_STATS_DISPLAY()
#endif

#if ICMP_STATS
#define ICMP_STATS_INC(x) STATS_INC(x)
#define ICMP_STATS_DISPLAY() stats_display_proto(&lwip_stats.icmp, "ICMP")
#else
#define ICMP_STATS_INC(x)
#define ICMP_STATS_DISPLAY()
#endif

#if IGMP_STATS
#define IGMP_STATS_INC(x) STATS_INC(x)
#define IGMP_STATS_DISPLAY() stats_display_igmp(&lwip_stats.igmp, "IGMP")
#else
#define IGMP_STATS_INC(x)
#define IGMP_STATS_DISPLAY()
#endif

#if IP_STATS
#define IP_STATS_INC(x) STATS_INC(x)
#define IP_STATS_DISPLAY() stats_display_proto(&lwip_stats.ip, "IP")
#else
#define IP_STATS_INC(x)
#define IP_STATS_DISPLAY()
#endif

#if IPFRAG_STATS
#define IPFRAG_STATS_INC(x) STATS_INC(x)
#define IPFRAG_STATS_DISPLAY() stats_display_proto(&lwip_stats.ip_frag, "IP_FRAG")
#else
#define IPFRAG_STATS_INC(x)
#define IPFRAG_STATS_DISPLAY()
#endif

#if ETHARP_STATS
#define ETHARP_STATS_INC(x) STATS_INC(x)
#define ETHARP_STATS_DISPLAY() stats_display_proto(&lwip_stats.etharp, "ETHARP")
#else
#define ETHARP_STATS_INC(x)
#define ETHARP_STATS_DISPLAY()
#endif

#if LINK_STATS
#define LINK_STATS_INC(x) STATS_INC(x)
#define LINK_STATS_DISPLAY() stats_display_proto(&lwip_stats.link, "LINK")
#else
#define LINK_STATS_INC(x)
#define LINK_STATS_DISPLAY()
#endif

#if MEMP_STATS
#define MEMP_STATS_AVAIL(x, i, y) lwip_stats.memp[i].x = y
#define MEMP_STATS_INC(x, i) STATS_INC(memp[i].x)
#define MEMP_STATS_DEC(x, i) STATS_DEC(memp[i].x)
#define MEMP_STATS_INC_USED(x, i) STATS_INC_USED(memp[i], 1)
#define MEMP_STATS_DISPLAY(i) stats_display_memp(&lwip_stats.memp[i], i)
#else
#define MEMP_STATS_AVAIL(x, i, y)
#define MEMP_STATS_INC(x, i)
#define MEMP_STATS_DEC(x, i)
#define MEMP_STATS_INC_USED(x, i)
#define MEMP_STATS_DISPLAY(i)
#endif

#if IP6_STATS
#define IP6_STATS_INC(x) STATS_INC(x)
#define IP6_STATS_DISPLAY() stats_display_proto(&lwip_stats.ip6, "IPv6")
#else
#define IP6_STATS_INC(x)
#define IP6_STATS_DISPLAY()
#endif

#if ICMP6_STATS
#define ICMP6_STATS_INC(x) STATS_INC(x)
#define ICMP6_STATS_DISPLAY() stats_display_proto(&lwip_stats.icmp6, "ICMPv6")
#else
#define ICMP6_STATS_INC(x)
#define ICMP6_STATS_DISPLAY()
#endif

#if IP6_FRAG_STATS
#define IP6_FRAG_STATS_INC(x) STATS_INC(x)
#define IP6_FRAG_STATS_DISPLAY() stats_display_proto(&lwip_stats.ip6_frag, "IPv6 FRAG")
#else
#define IP6_FRAG_STATS_INC(x)
#define IP6_FRAG_STATS_DISPLAY()
#endif

#if MLD6_STATS
#define MLD6_STATS_INC(x) STATS_INC(x)
#define MLD6_STATS_DISPLAY() stats_display_igmp(&lwip_stats.mld6, "MLDv1")
#else
#define MLD6_STATS_INC(x)
#define MLD6_STATS_DISPLAY()
#endif

#if ND6_STATS
#define ND6_STATS_INC(x) STATS_INC(x)
#define ND6_STATS_DISPLAY() stats_display_proto(&lwip_stats.nd6, "ND")
#else
#define ND6_STATS_INC(x)
#define ND6_STATS_DISPLAY()
#endif

#if MIB2_STATS
#define MIB2_STATS_INC(x) STATS_INC(x)
#else
#define MIB2_STATS_INC(x)
#endif

/* Display of statistics */
#if LWIP_STATS_DISPLAY
void stats_display(void);
void stats_display_proto(struct stats_proto *proto, const char *name);
void stats_display_igmp(struct stats_igmp *igmp, const char *name);
void stats_display_mem(struct stats_mem *mem, const char *name);
void stats_display_memp(struct stats_mem *mem, int index);
#else /* LWIP_STATS_DISPLAY */
#define stats_display()
#define stats_display_proto(proto, name)
#define stats_display_igmp(igmp, name)
#define stats_display_mem(mem, name)
#define stats_display_memp(mem, index)
#endif /* LWIP_STATS_DISPLAY */

#ifdef __cplusplus
}
#endif

#endif /* LWIP_HDR_STATS_H */
