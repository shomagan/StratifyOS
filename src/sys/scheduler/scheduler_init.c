/* Copyright 2011-2016 Tyler Gilbert; 
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

/*! \addtogroup SCHED
 * @{
 *
 */

/*! \file */
#include "config.h"
#include "scheduler_local.h"

#include "mcu/mcu.h"
#include "mcu/core.h"
#include "mcu/debug.h"

volatile scheduler_fault_t m_scheduler_fault MCU_SYS_MEM;

/*! \details This function initializes the peripheral hardware needed
 * by the scheduler specifically the microsecond timer.
 * \return Zero on success or an error code (see \ref caoslib_err_t)
 */
int scheduler_init(){
	task_table = sos_task_table;

	m_scheduler_current_priority = SCHED_LOWEST_PRIORITY - 1;
	m_scheduler_status_changed = 0;

	memset((void*)task_table, 0, sizeof(task_t) * sos_board_config.task_total);
	memset((void*)sos_sched_table, 0, sizeof(sched_task_t) * sos_board_config.task_total);

	//Do basic init of task 0 so that memory allocation can happen before the scheduler starts
	task_table[0].reent = _impure_ptr;
	task_table[0].global_reent = _global_impure_ptr;

	return 0;
}

/* \details This function initializes the scheduler.  It should be
 * called after all peripherals are initialized and interrupts are on.
 * \return Zero on success or an error code
 */
int scheduler_start(void * (*init)(void*), int priority){

	sos_sched_table[0].init = init;
	sos_sched_table[0].priority = priority;
	sos_sched_table[0].attr.stackaddr = &_data;
	sos_sched_table[0].attr.stacksize = sos_board_config.sys_memory_size;

	//Start the scheduler in a new thread
	if ( task_init(SCHED_RR_DURATION,
			scheduler, //run the scheduler
			NULL, //Let the task init function figure out where the stack needs to be and the heap size
			sos_board_config.sys_memory_size)
	){
		return -1;
	}

	//Program never gets to this point
	return -1;
}


int scheduler_prepare(){

	if ( mcu_debug_init() < 0 ){
		cortexm_disable_interrupts(NULL);
		mcu_board_execute_event_handler(MCU_BOARD_CONFIG_EVENT_ROOT_FATAL, (void*)"dbgi");
	}

	mcu_debug_user_printf("MCU Debug start\n");

#if SCHED_USECOND_TMR_SLEEP_OC > -1
	if ( scheduler_timing_init() ){
		return -1;
	}
#endif

	mcu_debug_user_printf("Load MCU Faults\n");

	//Load any possible faults from the last reset
	mcu_fault_load((fault_t*)&m_scheduler_fault.fault);

	mcu_debug_user_printf("Init MPU\n");
	if ( task_init_mpu(&_data, sos_board_config.sys_memory_size) < 0 ){
		scheduler_debug("Failed to initialize memory protection\n");
		mcu_board_execute_event_handler(MCU_BOARD_CONFIG_EVENT_ROOT_FATAL, (void*)"tski");
	}

	cortexm_set_unprivileged_mode(); //Enter unpriv mode
	return 0;
}

/*! @} */
