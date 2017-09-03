//////#ifndef DEVICE_INTERFACE_H_16082016
//////#define DEVICE_INTERFACE_H_16082016
//////
//////#ifdef __cplusplus
//////extern "C" {
//////#endif
//////
//////#include "types.h"
//////#include "utility.h"
//////#include "SerialDebugger.h"
//////#include "fsys.h"
//////#include "memory.h"
//////
//////	// Device Control Function Pointer. Used to send commands directly to the underlying hardware
//////	typedef void* (*DCFP)(uint32, ...);
//////
//////	// File System Function Pointer. Used to process high level commands to prepare them for the DCFP.
//////	typedef void* (*FSFP)();
//////
//////	// represents the device info required by each device found
//////	typedef struct _DEVICE
//////	{
//////		// Device standard part
//////		char name[16];
//////		uint32 device_id;
//////
//////		DCFP device_control;
//////		FSFP file_sys_control;
//////
//////		_DEVICE* next;
//////
//////		///////////////////////////
//////
//////		uint8 device_specific_info[];
//////	}DEVICE, *PDEVICE;
//////
//////	// master header for the whole device manager list structure.
//////	typedef struct _DEVICE_HEADER
//////	{
//////		PDEVICE start;
//////		PDEVICE end;
//////		uint16 device_count;
//////	}DEVICE_HEADER;
//////
//////	// initializes the device header.
//////	void init_device_manager();
//////
//////	// allocates space for a new device and returns the allocated address so that data can be filled in.
//////	// info_size is the size required by the device specific info.
//////	PDEVICE mngr_device_add(uint32 info_size);
//////
//////	// adds the standard info parts of the device info structure.
//////	void mngr_device_add_std_info(PDEVICE device, char* dev_name, uint32 dev_id, DCFP dev_ctrl, FSFP fs_ctrl);
//////
//////	// removes a device by its ID
//////	void mngr_device_remove(uint32 device_id);
//////
//////	// returns a device by its ID while pushing it towards the start of the list. Used for faster access.
//////	PDEVICE mngr_device_get(uint32 device_id);
//////
//////	// prints the standard part of device list for debug purposes.
//////	void mngr_device_print();
//////
//////#ifdef __cplusplus
//////}
//////#endif
//////
//////#endif