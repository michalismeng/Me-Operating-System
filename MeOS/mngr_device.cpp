#include "mngr_device.h"
#include "print_utility.h"

// private data and functions
DEVICE_HEADER dev_header;

void mngr_device_clear()
{
	dev_header.start = dev_header.end = 0;
}

bool mngr_device_is_clear()
{
	return (dev_header.start == 0 && dev_header.end == 0);
}

// public functions

void init_device_manager()
{
	dev_header.start = dev_header.end = 0;
	dev_header.device_count = 0;
}

PDEVICE mngr_device_add(uint32 info_size)
{
	PDEVICE area = (PDEVICE)calloc(sizeof(DEVICE) + info_size);

	if (mngr_device_is_clear())
		dev_header.start = dev_header.end = area;
	else
	{
		dev_header.end->next = area;
		dev_header.end = area;
	}
	area->next = 0;

	dev_header.device_count++;

	return dev_header.end;
}

void mngr_device_add_std_info(PDEVICE device, char* dev_name, uint32 dev_id, DCFP dev_ctrl, FSFP fs_ctrl)
{
	if (device == 0)
		return;

	if (strcpy_s(device->name, 16, dev_name) != 0)
		return;

	device->device_id = dev_id;
	device->device_control = dev_ctrl;
	device->file_sys_control = fs_ctrl;
}

void mngr_device_remove(uint32 device_id)
{
	if (dev_header.device_count == 0)
		return;

	PDEVICE dev = dev_header.start, prev = 0;

	while (dev != 0)
	{
		if (dev->device_id == device_id)
		{
			if (dev == dev_header.start)		// match on first element
			{
				if (dev_header.device_count == 1)
					mngr_device_clear();
				else
					dev_header.start = dev->next;

				free(dev);
			}
			else if (dev == dev_header.end)		// match on last element
			{
				if (dev_header.device_count == 1)
					mngr_device_clear();
				else
					dev_header.end = prev;

				free(dev);
			}
			else								// intermediate match
			{
				PDEVICE temp = dev->next;
				free(dev);
				prev->next = temp->next;
			}

			dev_header.device_count--;
			break;
		}

		prev = dev;
		dev = dev->next;
	}
}

PDEVICE mngr_device_get(uint32 device_id)
{
	if (dev_header.device_count == 0)
		return 0;

	PDEVICE dev = dev_header.start, prev = 0, prev_prev = 0;

	while (dev != 0)
	{
		if (dev->device_id == device_id)
		{
			if (dev == dev_header.start)	// dev is the first, so no swap
				return dev;

			if (prev == dev_header.start)
			{
				PDEVICE temp = dev->next;

				dev->next = prev;
				dev_header.start = dev;
				prev->next = temp;

				if (dev == dev_header.end)	// dev is the last entry (case of two enties in list)
					dev_header.end = prev;

				return dev;
			}

			PDEVICE temp = dev->next;

			prev_prev->next = dev;
			dev->next = prev;
			prev->next = temp;

			if (dev == dev_header.end)	// dev is the last entry
				dev_header.end = prev;

			return dev;
		}

		prev_prev = prev;
		prev = dev;
		dev = dev->next;
	}
}

void mngr_device_print()
{
	printfln("Device manager has: %u devices.", dev_header.device_count);
	PDEVICE dev = dev_header.start;

	while (dev != 0)
	{
		printfln("device: %s ID: %x next: %h", dev->name, dev->device_id, dev->next);
		dev = dev->next;
	}
}