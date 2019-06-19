
#include <sos/dev/qspi.h>
#include <sos/dev/spi.h>
#include <mcu/pio.h>
#include <mcu/debug.h>

#include "device/drive_cfi.h"

#if !defined I_SWAP
#define I_SWAP I_SPI_SWAP
#define I_SETATTR I_SPI_SETATTR
#endif

#if 0
enum cfi_instructions {
	INSTRUCTION_WRITE_ENABLE = 0x06,
	INSTRUCTION_VOLATILE_SR_WRITE_ENABLE = 0x50,
	INSTRUCTION_WRITE_DISABLE = 0x04,
	INSTRUCTION_RELEASE_POWER_DOWN_ID = 0xAB,
	INSTRUCTION_MANUFACTURER_DEVICE_ID = 0x90,
	INSTRUCTION_JEDEC_ID = 0x9F,
	INSTRUCTION_READ_UNIQUE_ID = 0x48,
	INSTRUCTION_READ_DATA = 0x03,
	INSTRUCTION_FAST_READ = 0x0B,
	INSTRUCTION_PAGE_PROGRAM = 0x02,
	INSTRUCTION_SECTOR_ERASE_4KB = 0x20,
	INSTRUCTION_BLOCK_ERASE_32KB = 0x52,
	INSTRUCTION_BLOCK_ERASE_64KB = 0xD8,
	INSTRUCTION_CHIP_ERASE = 0xC7,
	INSTRUCTION_READ_STATUS_1 = 0x05,
	INSTRUCTION_WRITE_STATUS_1 = 0x01,
	INSTRUCTION_READ_STATUS_2 = 0x35,
	INSTRUCTION_WRITE_STATUS_2 = 0x31,
	INSTRUCTION_READ_STATUS_3 = 0x15,
	INSTRUCTION_WRITE_STATUS_3 = 0x11,

	INSTRUCTION_READ_SFPD_REGISTER = 0x5A,
	INSTRUCTION_ERASE_SECURITY_REGISTER = 0x44,
	INSTRUCTION_PROGRAM_SECURITY_REGISTER = 0x42,
	INSTRUCTION_READ_SECURITY_REGISTER = 0x48,
	INSTRUCTION_GLOBAL_BLOCK_LOCK = 0x7E,
	INSTRUCTION_GLOBAL_BLOCK_UNLOCK = 0x7A,
	INSTRUCTION_POWERDOWN = 0xB9,
	INSTRUCTION_ENABLE_RESET = 0x66,
	INSTRUCTION_RESET_DEVICE = 0x99,

	//dual/quad spi instructions
	INSTRUCTION_FAST_READ_DUAL_OUTPUT = 0x3B,
	INSTRUCTION_FAST_READ_DUAL_IO = 0xBB,
	INSTRUCTION_MANUFACTURER_DEVICE_ID_DUAL_IO = 0x92,
	INSTRUCTION_QUAD_INPUT_PAGE_PROGRAM = 0x32,
	INSTRUCTION_FAST_READ_QUAD_OUTPUT = 0x6B,
	INSTRUCTION_MANUFACTURER_DEVICE_ID_QUAD_IO = 0x94,
	INSTRUCTION_FAST_READ_QUAD_IO = 0xEB,
	INSTRUCTION_SET_BURST_WITH_WRAP = 0x77,
};
#endif


static int drive_cfi_write_instruction(
		const devfs_handle_t * handle,
		u8 instruction,
		const u8 * data,
		u8 data_size);

static int drive_cfi_write_instruction_with_cs(
		const devfs_handle_t * handle,
		u8 instruction,
		const u8 * data,
		u8 data_size);

static u8 drive_cfi_read_status_with_cs(const devfs_handle_t * handle);

static void drive_cfi_initialize_cs(const devfs_handle_t * handle);
static void drive_cfi_assert_cs(const devfs_handle_t * handle);
static void drive_cfi_deassert_cs(const devfs_handle_t * handle);
static int drive_cfi_handle_complete(void * context, const mcu_event_t * event);


int drive_cfi_spi_open(const devfs_handle_t * handle){
	int result;
	const drive_cfi_config_t * config = handle->config;

	result = config->serial_device->driver.open(&config->serial_device->handle);
	if( result < 0 ){ return result; }

	//load device status

	return SYSFS_RETURN_SUCCESS;
}

int drive_cfi_spi_ioctl(const devfs_handle_t * handle, int request, void * ctl){
	const drive_cfi_config_t * config = handle->config;
	drive_attr_t * attr = ctl;
	drive_info_t * info = ctl;
	int result;
	u8 status;

	switch(request){
		case I_DRIVE_GETVERSION: return DRIVE_VERSION;

		case I_DRIVE_SETATTR:
			{
				if( attr == 0 ){ return SYSFS_SET_RETURN(EINVAL); }
				u32 o_flags = attr->o_flags;

				if( o_flags & DRIVE_FLAG_INIT ){
					//execute powerup sequence

					//init the CS pin if it is available
					drive_cfi_initialize_cs(handle);

					//set serial driver attributes to defaults
					result= config->serial_device->driver.ioctl(
								&config->serial_device->handle,
								I_SETATTR,
								0);

					if( result < 0 ){
						return result;
					}

				}

				if( o_flags & DRIVE_FLAG_PROTECT ){
					drive_cfi_write_instruction_with_cs(handle, config->opcode.write_enable, 0, 0);
					drive_cfi_write_instruction_with_cs(handle, config->opcode.protect, 0, 0);
				}

				if( o_flags & DRIVE_FLAG_UNPROTECT ){
					drive_cfi_write_instruction_with_cs(handle, config->opcode.write_enable, 0, 0);
					drive_cfi_write_instruction_with_cs(handle, config->opcode.unprotect, 0, 0);
				}

				if( o_flags & DRIVE_FLAG_RESET ){
					drive_cfi_write_instruction_with_cs(handle, config->opcode.enable_reset, 0, 0);
					drive_cfi_write_instruction_with_cs(handle, config->opcode.reset, 0, 0);
				}

				if( o_flags & DRIVE_FLAG_POWERUP ){
					drive_cfi_write_instruction_with_cs(handle, config->opcode.power_up, 0, 0);

				}

				if( o_flags & DRIVE_FLAG_POWERDOWN ){
					drive_cfi_write_instruction_with_cs(handle, config->opcode.power_down, 0, 0);
				}

				if( o_flags & DRIVE_FLAG_ERASE_BLOCKS ){
					//erase the smallest possible section size
					u8 address[3];
					for(u32 i=attr->start; i < attr->end; i+=4096){
						address[0] = i >> 16;
						address[1] = i >> 8;
						address[0] = i;
						drive_cfi_write_instruction_with_cs(handle, config->opcode.block_erase, address, sizeof(address));
					}

				}

				if( o_flags & DRIVE_FLAG_ERASE_DEVICE ){
					drive_cfi_write_instruction_with_cs(handle, config->opcode.device_erase, 0, 0);
				}


			}
			break;

		case I_DRIVE_GETINFO:
			info->o_flags = DRIVE_FLAG_INIT |
					DRIVE_FLAG_RESET |
					DRIVE_FLAG_PROTECT |
					DRIVE_FLAG_UNPROTECT |
					DRIVE_FLAG_ERASE_BLOCKS |
					DRIVE_FLAG_ERASE_DEVICE |
					DRIVE_FLAG_POWERUP |
					DRIVE_FLAG_POWERDOWN |
					0;

			info->o_events = MCU_EVENT_FLAG_WRITE_COMPLETE | MCU_EVENT_FLAG_DATA_READY;
			info->address_size = config->info.address_size; //one byte for each address location
			info->write_block_size = config->info.write_block_size; //can write one byte at a time
			info->num_write_blocks = config->info.num_write_blocks;
			info->erase_block_size = config->info.erase_block_size;
			info->erase_block_time = config->info.erase_block_time;
			info->erase_device_time = config->info.erase_device_time;
			info->bitrate = config->info.bitrate;
			break;

		case I_DRIVE_ISBUSY:
			status = drive_cfi_read_status_with_cs(handle);
			if( (status & config->opcode.busy_status_mask) != 0 ){
				//device is busy
				return 1;
			}
			break;

	}

	return SYSFS_RETURN_SUCCESS;
}

int drive_cfi_handle_complete(void * context, const mcu_event_t * event){
	const devfs_handle_t * handle = context;
	drive_cfi_state_t * state = handle->state;

	//operation is complete

	//deassert the cs
	drive_cfi_deassert_cs(handle);

	devfs_execute_event_handler(&state->handler, MCU_EVENT_FLAG_WRITE_COMPLETE, 0);
	state->handler.callback = 0;
	return 0;
}

int drive_cfi_spi_read(const devfs_handle_t * handle, devfs_async_t * async){
	int result;
	const drive_cfi_config_t * config = handle->config;
	drive_cfi_state_t * state = handle->state;

	//is device already busy?
	if( state->handler.callback != 0 ){ return SYSFS_SET_RETURN(EBUSY); }


	//assert CS for the page program and data write
	drive_cfi_assert_cs(handle);

	//read instruction
	u32 address = async->loc & 0x00ffffff;
	u8 fast_read[4];
	fast_read[0] = address >> 16;
	fast_read[1] = address >> 8;
	fast_read[2] = address;
	fast_read[3] = 0; //dummy byte
	drive_cfi_write_instruction(handle, config->opcode.fast_read, fast_read, sizeof(fast_read));

	//hi-jack the callback handler and restore it laster
	state->handler = async->handler;
	async->handler.callback = drive_cfi_handle_complete;
	async->handler.context = (void*)handle;
	result = config->serial_device->driver.read(&config->serial_device->handle, async);
	if( result != 0 ){
		drive_cfi_deassert_cs(handle);
	}

	return result;
}



int drive_cfi_spi_write(const devfs_handle_t * handle, devfs_async_t * async){
	int result;
	const drive_cfi_config_t * config = handle->config;
	drive_cfi_state_t * state = handle->state;

	//is device already busy?
	if( state->handler.callback != 0 ){ return SYSFS_SET_RETURN(EBUSY); }

	u32 page_size = config->opcode.page_program_size;
	u32 page_program_mask = page_size-1;

	if( ((async->loc & page_program_mask) + async->nbyte) > page_size ){
		//allow a partial page program but don't allow overflow
		async->nbyte = page_size - (async->loc & page_program_mask);
	}

	//write enable instruction
	drive_cfi_write_instruction_with_cs(handle, config->opcode.write_enable, 0, 0);

	//assert CS for the page program and data write
	drive_cfi_assert_cs(handle);

	//page program instruction
	u32 address = async->loc & 0x00ffffff;
	u8 page_program[3];
	page_program[0] = address >> 16;
	page_program[1] = address >> 8;
	page_program[2] = address;
	drive_cfi_write_instruction(handle, config->opcode.page_program, page_program, sizeof(page_program));

	//hi-jack the callback handler and restore it laster
	state->handler = async->handler;
	async->handler.callback = drive_cfi_handle_complete;
	async->handler.context = (void*)handle;
	result = config->serial_device->driver.write(&config->serial_device->handle, async);
	if( result != 0 ){
		drive_cfi_deassert_cs(handle);
	}
	return result;
}

int drive_cfi_spi_close(const devfs_handle_t * handle){
	return mcu_spi_close(handle);
}

void drive_cfi_initialize_cs(const devfs_handle_t * handle){
	const drive_cfi_config_t * config = handle->config;
	mcu_pin_t cs = config->cs;
	if( cs.port != 255 ){
		devfs_handle_t pio_handle;
		pio_attr_t pio_attr;
		pio_attr.o_pinmask = 1<<cs.pin;
		pio_attr.o_flags = PIO_FLAG_SET_OUTPUT;
		pio_handle.port = cs.port;
		pio_handle.config = 0;
		mcu_pio_setattr(&pio_handle, &pio_attr);
	}
}

void drive_cfi_assert_cs(const devfs_handle_t * handle){
	const drive_cfi_config_t * config = handle->config;
	mcu_pin_t cs = config->cs;
	if( cs.port != 255 ){
		devfs_handle_t pio_handle;
		pio_handle.port = cs.port;
		pio_handle.config = 0;
		mcu_pio_clrmask(&pio_handle, (void*)(ssize_t)(1<<cs.pin));
	}
}

void drive_cfi_deassert_cs(const devfs_handle_t * handle){
	const drive_cfi_config_t * config = handle->config;
	mcu_pin_t cs = config->cs;
	if( cs.port != 255 ){
		devfs_handle_t pio_handle;
		pio_handle.port = cs.port;
		pio_handle.config = 0;
		mcu_pio_setmask(&pio_handle, (void*)(ssize_t)(1<<cs.pin));
	}
}

int drive_cfi_write_instruction_with_cs(
		const devfs_handle_t * handle,
		u8 instruction,
		const u8 * data,
		u8 data_size){

	//assert cs
	drive_cfi_assert_cs(handle);

	drive_cfi_write_instruction(handle, instruction, data, data_size);

	//deassert cs
	drive_cfi_deassert_cs(handle);

	return 0;
}

u8 drive_cfi_read_status_with_cs(const devfs_handle_t * handle){
	const drive_cfi_config_t * config = handle->config;
	u8 result;
#if I_SWAP == I_SPI_SWAP
	result = config->serial_device->driver.ioctl(&config->serial_device->handle, I_SWAP, (void*)(u32)config->opcode.read_busy_status);
#else

#endif

	return result;
}

int drive_cfi_write_instruction(
		const devfs_handle_t * handle,
		u8 instruction,
		const u8 * data,
		u8 data_size){
	const drive_cfi_config_t * config = handle->config;

#if I_SWAP == I_SPI_SWAP
	config->serial_device->driver.ioctl(&config->serial_device->handle, I_SWAP, (void*)(u32)instruction);
	for(u8 i=0; i < data_size; i++){
		config->serial_device->driver.ioctl(
					&config->serial_device->handle,
					I_SWAP,
					(void*)(u32)data[i]);
	}
#else
	config->serial_device->driver.ioctl(&config->serial_device->handle, I_SWAP, (void*)(u32)instruction);
	mcu_spi_swap(handle, (void*)(u32)instruction);
	for(u8 i=0; i < data_size; i++){
		config->serial_device->driver.ioctl(
					&config->serial_device->handle,
					I_SWAP,
					(void*)(u32)data[i]);
	}

#endif

	return 0;
}
