/*
  rcutil

  Copyright (C) 2019 Sony Computer Science Laboratories
  Author(s) Peter Hanappe

  rcutil is light-weight libary for inter-node communication.

  rcutil is free software: you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as
  published by the Free Software Foundation, either version 3 of the
  License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <http://www.gnu.org/licenses/>.

 */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include <rcom.h>

// Doesn't list Arduino device:
// ls -l /sys/bus/usb-serial/devices/
// ls -l /dev/serial/by-id/


typedef struct _device_id_t {
        int vendor_id;
        int product_id;
        const char *name;
} device_id_t;

// 2341:0043 Arduino SA Uno R3

static libusb_device_handle *inspect_usb_device(libusb_device *device)
{
        struct libusb_device_descriptor	desc;
        libusb_device_handle *handle = NULL;
        int ret;
        int serial;
        int id_interface = 0;
        
        ret = libusb_get_device_descriptor(device, &desc);                
        if (ret != 0) {
                r_err("libusb_get_device_descriptor failed with error code: %d", ret);
                return NULL;
        }

        r_debug("idVendor %x, idProduct %x", desc.idVendor, desc.idProduct);

        return NULL;
        
        /* ret = libusb_open(device, &handle); */
        /* if (ret != 0) { */
        /*         libusb_close(handle); */
        /*         r_err("libusb_open failed with error code: %d", ret); */
        /*         r_err("This usually means you need to run as root, or install the udev rules."); */
        /*         return NULL; */
        /* } */
        
        /* /\* ret = check_kernel_driver(handle, id_interface); *\/ */
        /* /\* if (ret != 0) { *\/ */
        /* /\*         libusb_close(handle); *\/ */
        /* /\*         return NULL; *\/ */
        /* /\* } *\/ */
        
        /* ret = libusb_claim_interface(handle, id_interface); */
        /* if (ret != 0) { */
        /*         if (ret == LIBUSB_ERROR_BUSY) */
        /*                 r_warn("libusb_claim_interface failed with BUSY - " */
        /*                          "probably the device is opened by another program."); */
        /*         else */
        /*                 r_err("libusb_claim_interface failed with error code: %d", ret); */
        /*         libusb_close(handle); */
        /*         return NULL; */
        /* } */

        /* // keep it. */
        /* return handle; */
}

int usbdev_init()
{
        libusb_device_handle *handle = NULL;
	libusb_device **list = NULL;

        int r = libusb_init(NULL);
        if (r < 0) {
                r_err("Failed to initialize libusb");
                return -1;
        }

        ssize_t size = libusb_get_device_list(NULL, &list);
        
	if (size < 0)
		r_err("libusb_get_device_list failed with error code: %d", size);
	
	for (int i = 0; i < size; i++) {
                handle = inspect_usb_device(list[i]);
                if (handle != NULL)
                        break;
        }
        
	if (list)	
		libusb_free_device_list(list, 1);

        return 0;
}

int test_grbl(serial_t *serial)
{
        if (serial_flush(serial) != 0)
                return -1;

        if (serial_println(serial, "$I") != 0) {
                r_err("test_grbl: serial_println failed");
                return -1;
        }
        
        membuf_t *buffer = new_membuf();
        if (serial_readline(serial, buffer) == NULL) {
                r_err("test_grbl: serial_readline failed");
                delete_membuf(buffer);
                return -1;
        }
        int ret = (strncmp(membuf_data(buffer), "[VER:", 5) == 0);

        delete_membuf(buffer);
        return ret;
}

int test_brush_motor(serial_t *serial)
{
        if (serial_flush(serial) != 0)
                return -1;
        if (serial_println(serial, "?") != 0) {
                r_err("test_grbl: serial_println failed");
                return -1;
        }
        
        membuf_t *buffer = new_membuf();
        if (serial_readline(serial, buffer) == NULL) {
                r_err("test_grbl: serial_readline failed");
                delete_membuf(buffer);
                return -1;
        }

        int ret = (strstr(membuf_data(buffer), "RomiMotorController") != NULL);

        delete_membuf(buffer);
        return ret;
}

int test_device(const char *device)
{
        serial_t *serial = new_serial(device, 115200);
        if (serial == NULL)
                return -1;

        if (test_grbl(serial) == 1) {
                r_debug("device '%s' runs grbl", device);
        } else if (test_brush_motor(serial) == 1) {
                r_debug("device '%s' runs the brush motor controller", device);
        }
        delete_serial(serial);
        return 0;
}

int test_devices()
{
        const char *base[] = { "/dev/ttyACM", "/dev/ttyUSB", NULL };
        struct stat statbuf;
        char path[200];

        for (int i = 0; base[i] != NULL; i++) {
                for (int j = 0; j < 10; j++) {
                        rprintf(path, sizeof(path), "%s%d", base[i], j);
                        int r = stat(path, &statbuf);
                        if (r != 0
                            || (statbuf.st_mode & S_IFMT) != S_IFCHR)
                                continue;
                        r_debug("%s", path);
                        test_device(path);
                }
        }
        return 0;
}

int main(int argc, char **argv)
{
        app_init(&argc, argv);
        usbdev_init();
        test_devices();
}
