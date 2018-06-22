/****************************************************************************
 * net/udp/udp_finddev.c
 *
 *   Copyright (C) 2015 Gregory Nutt. All rights reserved.
 *   Author: Gregory Nutt <gnutt@nuttx.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name NuttX nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#if defined(CONFIG_NET) && defined(CONFIG_NET_UDP)

#include <string.h>

#include <nuttx/net/netdev.h>
#include <nuttx/net/ip.h>

#include "netdev/netdev.h"
#include "inet/inet.h"
#include "udp/udp.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: udp_find_ipv4_device
 *
 * Description:
 *   Select the network driver to use with the IPv4 UDP transaction.
 *
 * Input Parameters:
 *   conn - UDP connection structure (not currently used).
 *   ipv4addr - The IPv4 address to use in the device selection.
 *
 * Returned Value:
 *   A pointer to the network driver to use.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv4
FAR struct net_driver_s *udp_find_ipv4_device(FAR struct udp_conn_s *conn,
                                              in_addr_t ipv4addr)
{
  /* Return NULL if the address is INADDR_ANY.  In this case, there may
   * be multiple devices that can provide data so the exceptional events
   * from any particular device are not important.
   *
   * Of course, it would be a problem if this is the remote address of
   * sendto().
   */

  if (net_ipv4addr_cmp(ipv4addr, INADDR_ANY))
    {
      return NULL;
    }

  /* We need to select the device that is going to route the UDP packet
   * based on the provided IP address.
   */

  return netdev_findby_ipv4addr(conn->u.ipv4.laddr, ipv4addr);
}
#endif /* CONFIG_NET_IPv4 */

/****************************************************************************
 * Name: udp_find_ipv6_device
 *
 * Description:
 *   Select the network driver to use with the IPv6 UDP transaction.
 *
 * Input Parameters:
 *   conn - UDP connection structure (not currently used).
 *   ipv6addr - The IPv6 address to use in the device selection.
 *
 * Returned Value:
 *   A pointer to the network driver to use.
 *
 ****************************************************************************/

#ifdef CONFIG_NET_IPv6
FAR struct net_driver_s *udp_find_ipv6_device(FAR struct udp_conn_s *conn,
                                              net_ipv6addr_t ipv6addr)
{
  /* Return NULL if the address is IN6ADDR_ANY.  In this case, there may
   * be multiple devices that can provide data so the exceptional events
   * from any particular device are not important.
   *
   * Of course, it would be a problem if this is the remote address of
   * sendto().
   */

  if (net_ipv6addr_cmp(ipv6addr, g_ipv6_allzeroaddr))
    {
      return NULL;
    }

  /* We need to select the device that is going to route the UDP packet
   * based on the provided IP address.
   */

  return netdev_findby_ipv6addr(conn->u.ipv6.laddr, ipv6addr);
}
#endif /* CONFIG_NET_IPv6 */

/****************************************************************************
 * Name: udp_find_laddr_device
 *
 * Description:
 *   Select the network driver to use with the UDP transaction using the
 *   locally bound IP address.
 *
 * Input Parameters:
 *   conn - UDP connection structure (not currently used).
 *
 * Returned Value:
 *   A pointer to the network driver to use.
 *
 ****************************************************************************/

FAR struct net_driver_s *udp_find_laddr_device(FAR struct udp_conn_s *conn)
{
  /* There are multiple network devices.  We need to select the device that
   * is going to route the UDP packet based on the provided IP address.
   */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
      if (conn->domain == PF_INET)
#endif
        {
          return udp_find_ipv4_device(conn, conn->u.ipv4.laddr);
        }
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
      else
#endif
        {
          return udp_find_ipv6_device(conn, conn->u.ipv6.laddr);
        }
#endif
}

/****************************************************************************
 * Name: udp_find_raddr_device
 *
 * Description:
 *   Select the network driver to use with the UDP transaction using the
 *   remote IP address.
 *
 * Input Parameters:
 *   conn - UDP connection structure.
 *
 * Returned Value:
 *   A pointer to the network driver to use.
 *
 ****************************************************************************/

FAR struct net_driver_s *udp_find_raddr_device(FAR struct udp_conn_s *conn)
{
  /* We need to select the device that is going to route the UDP packet
   * based on the provided IP address.
   */

#ifdef CONFIG_NET_IPv4
#ifdef CONFIG_NET_IPv6
      if (conn->domain == PF_INET)
#endif
        {
          /* Check if the remote, destination address is the broadcast
           * or multicast address.  In this is the case select the device
           * using the locally bound address.
           *
           * REVISIT:  This logic is to handle the case where UDP broadcast
           * is used when there are multiple devices.  In this case we can
           * select the device using the bound address address.  It is a
           * kludge for the unsupported SO_BINDTODEVICE socket option
           */

          if (conn->u.ipv4.raddr == INADDR_BROADCAST ||
              IN_MULTICAST(conn->u.ipv4.raddr))
            {
              return udp_find_ipv6_device(conn, conn->u.ipv6.laddr);
            }
          else
            {
              return udp_find_ipv4_device(conn, conn->u.ipv4.raddr);
            }
        }
#endif

#ifdef CONFIG_NET_IPv6
#ifdef CONFIG_NET_IPv4
      else
#endif
        {
          /* Check if the remote, destination address is a multicast
           * address.  In this is the case select the device
           * using the locally bound address.  The general form of an
           * reserved IPv6 multicast address is:  ff0x::xx/16.
           *
           * REVISIT:  This logic is to handle the case where UDP broadcast
           * is used when there are multiple devices.  In this case we can
           * select the device using the bound address address.  It is a
           * kludge for the unsupported SO_BINDTODEVICE socket option
           */

          if ((conn->u.ipv6.laddr[0] & HTONS(0xfff0)) == HTONS(0xff00) &&
              conn->u.ipv6.laddr[1] == 0 && conn->u.ipv6.laddr[2] == 0 &&
              conn->u.ipv6.laddr[3] == 0 && conn->u.ipv6.laddr[4] == 0 &&
              conn->u.ipv6.laddr[5] == 0 && conn->u.ipv6.laddr[6] == 0)
            {
              return udp_find_ipv6_device(conn, conn->u.ipv6.laddr);
            }
          else
            {
              return udp_find_ipv6_device(conn, conn->u.ipv6.raddr);
            }
        }
#endif
}

#endif /* CONFIG_NET && CONFIG_NET_UDP */
