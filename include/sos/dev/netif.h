/* Copyright 2011-2018 Tyler Gilbert;
 * This file is part of Stratify OS.
 *
 * Stratify OS is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Stratify OS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Stratify OS.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 */

/*! \addtogroup NETIF Network Interface
 * @{
 *
 *
 * \ingroup IFACE_NETIF_DEV
 *
 * \details This is the interface for network devices. These requests must be implemented
 * by any network interface (ethernet, wifi, etc) in order for LWIP to be able
 * to use the network interface.
 *
 */

/*! \file  */

#ifndef SOS_DEV_NETIF_H_
#define SOS_DEV_NETIF_H_

#include "mcu/types.h"

#define NETIF_VERSION (0x030000)
#define NETIF_IOC_CHAR 'N'

enum {
	NETIF_FLAG_INIT /*! When setting attributes, initializes the interface */ = (1<<0),
	NETIF_FLAG_DEINIT /*! When setting attributes, de-initialies the interface */ = (1<<1),
	NETIF_FLAG_IS_LINK_UP /*! When settings attributes, ioctl() return 1 if link is up */ = (1<<2),
	NETIF_FLAG_SET_LINK_UP /*! When setting attributes, sets the link as up */ = (1<<3),
	NETIF_FLAG_SET_LINK_DOWN /*! When setting attributes, sets the link as down */ = (1<<4)
};

typedef struct MCU_PACK {
	u32 o_flags;
	u32 o_events;
} netif_info_t;

/*! \brief Network Interface attributes
 */
typedef struct MCU_PACK {
	u32 o_flags;
	u16 mtu;
	u8 mac_address[10];
} netif_attr_t;


#define I_NETIF_GETVERSION _IOCTL(NETIF_IOC_CHAR, I_MCU_GETVERSION)

/*! \brief See below for details.
 * \details This requests reads the ADC attributes.
 *
 * Example:
 * \code
 * #include <sos/dev/netif.h>
 * netif_attr_t attr;
 * int netif_fd;
 * ...
 * ioctl(netif_fd, I_NETIF_GETINFO, &attr);
 * \endcode
 * \hideinitializer
 */
#define I_NETIF_GETINFO _IOCTLR(NETIF_IOC_CHAR, I_MCU_GETINFO, netif_info_t)

/*! \brief See below for details.
 * \details This requests writes the ETH attributes.
 *
 * Example:
 * \code
 * #include <sos/dev/netif.h>
 * netif_attr_t attr;
 * int netif_fd;
 * ...
 * ioctl(netif_fd, I_NETIF_SETATTR, &attr);
 * \endcode
 * \hideinitializer
 */
#define I_NETIF_SETATTR _IOCTLW(NETIF_IOC_CHAR, I_MCU_SETATTR, netif_attr_t)
#define I_NETIF_SETACTION _IOCTLW(NETIF_IOC_CHAR, I_MCU_SETACTION, mcu_action_t)

#define I_NETIF_TOTAL 0

#endif /* SOS_DEV_NETIF_H_ */

/*! @} */
