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

/*! \addtogroup EMC External Memory Controller (EMC)
 * @{
 *
 * \ingroup IFACE_DEV
 *
 * \details The EMC API allows configuration of the external memory controller.
 *
 * The EMC is usually initialized by the OS at startup using the following sample code:
 *
 * \code
 *
 *
 * \endcode
 *
 */

/*! \file
 * \brief External Interrupt Header File
 *
 */


#ifndef SOS_DEV_EMC_H_
#define SOS_DEV_EMC_H_

#include "mcu/types.h"

#define EMC_VERSION (0x030000)
#define EMC_IOC_IDENT_CHAR 'E'
#define MAX_DATA_BUS_SIZE 32
#define MAX_ADDRESS_BUS_SIZE 26
#define MAX_NCS_NUM 4
#define MAX_BL_NUM  4

/*! \brief See below for details */
/*! \details These are the flags for mode in emc_attr_t */
typedef enum {
	EMC_FLAG_DISABLE = (1<<0),
	EMC_FLAG_ENABLE = (1<<1),
	EMC_FLAG_IS_SDRAM = (1<<2),
	EMC_FLAG_IS_PSRAM = (1<<3),
	EMC_FLAG_IS_SRAM = (1<<4),
	EMC_FLAG_IS_NOR = (1<<5),
	EMC_FLAG_IS_NAND = (1<<6),
	EMC_FLAG_IS_8B_ACCESS = (1<<7),
	EMC_FLAG_IS_16B_ACCESS = (1<<8),
    EMC_FLAG_IS_32B_ACCESS = (1<<9),
    EMC_FLAG_IS_AHB = (1<<10)	,	/*!< use only data bus without address*/
    EMC_FLAG_IS_PSRAM_BANK1 = (1<<11)	,	/*!< use subbank BANK1 - cs0*/
    EMC_FLAG_IS_PSRAM_BANK2 = (1<<12)	,	/*!< use subbank BANK2 - cs1*/
    EMC_FLAG_IS_PSRAM_BANK3 = (1<<13)	,	/*!< use subbank BANK3 - cs2*/
    EMC_FLAG_IS_PSRAM_BANK4 = (1<<14)	,	/*!< use subbank BANK4 - cs3*/
    EMC_FLAG_AHB_WRITE_DATA = (1<<15)	,	/*!< write data throuth ahb */
    EMC_FLAG_AHB_WRITE_REG =  (1<<16)	,	/*!< write reg  throuth ahb*/

}emc_flag_t;
typedef struct MCU_PACK {
    mcu_pin_t we;
    mcu_pin_t oe;
    mcu_pin_t bl[MAX_BL_NUM];
    mcu_pin_t nadv;
    mcu_pin_t nwait;
    mcu_pin_t ncs[MAX_NCS_NUM];
    mcu_pin_t address[MAX_ADDRESS_BUS_SIZE];
    mcu_pin_t data[MAX_DATA_BUS_SIZE];
} emc_pin_assignment_t;

typedef struct {
	u32 o_flags;
	u32 o_events;
	u32 freq;
	u32 base_address;
    u32 size;
    u8 data_bus_width;
	u32 resd[8];
} emc_info_t;

typedef struct MCU_PACK {
	u32 o_flags;
    u32 base_address;
    u32 size;
    u32 freq;
    u32 data_or_reg;    /*!< use for AHB command*/
    u8 data_bus_width;
    emc_pin_assignment_t pin_assignment;
    u32 resd[8];
} emc_attr_t;


#define I_EMC_GETVERSION _IOCTL(EMC_IOC_IDENT_CHAR, I_MCU_GETVERSION)

#define I_EMC_GETINFO _IOCTLR(EMC_IOC_IDENT_CHAR, I_MCU_GETINFO, emc_info_t)

/*! \brief IOCTL request to set the attributes
 */
#define I_EMC_SETATTR _IOCTLW(EMC_IOC_IDENT_CHAR, I_MCU_SETATTR, emc_attr_t)

#define I_EMC_SETACTION _IOCTLW(EMC_IOC_IDENT_CHAR, I_MCU_SETACTION, mcu_action_t)

#define I_EMC_TOTAL 0


#endif /* SOS_DEV_EMC_H_ */

/*! @} */
