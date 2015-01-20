#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#define DEV_VID 0x1FAC
#define DEV_PID 0x0150
#define DEV_MSG "\x55\x53\x42\x43\x12\x34\x56\x78\x24\x00\x00\x00\x80\x01\x08\xdf\x20\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"

extern "C" {
#ifdef __WIN32
#include <windows.h>
#include <libusbx-1.0/libusb.h>
#else
#include <libusb-1.0/libusb.h>
#endif
};

int usb_bulk_io(struct libusb_device_handle *handle, unsigned char ep, char *bytes, int size, int timeout) {
	int actual_length;
	int r;
	r = libusb_bulk_transfer(handle, ep, (unsigned char *) bytes, size, &actual_length, timeout);
	if (r == 0 || (r == LIBUSB_ERROR_TIMEOUT && actual_length > 0))
		return actual_length;
	return r;
}

unsigned char find_first_bulk_endpoint(struct libusb_config_descriptor *active_config, int direction, int interface) {
	int i, j;
	const struct libusb_interface_descriptor *alt;
	const struct libusb_endpoint_descriptor *ep;
	
	for (j = 0; j < active_config->bNumInterfaces; j++) {
		alt = &(active_config->interface[j].altsetting[0]);
		if (alt->bInterfaceNumber == interface) {
			for (i = 0; i < alt->bNumEndpoints; i++) {
				ep = &(alt->endpoint[i]);
				if (((ep->bmAttributes & LIBUSB_ENDPOINT_ADDRESS_MASK) == LIBUSB_TRANSFER_TYPE_BULK) &&
						((ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK) == direction)) 
					return ep->bEndpointAddress;
			}
		}
	}
	return 0;
}

int main() {
	libusb_context *ctx = NULL;
	static struct libusb_device_handle *devh = NULL;
	struct libusb_device *dev = NULL;
	static struct libusb_config_descriptor *active_config = NULL;

	setlocale(LC_ALL, "Russian");
	
	libusb_init(&ctx);
	libusb_set_debug(ctx, 3);
	
	struct libusb_device **devs;
	
	if (libusb_get_device_list(ctx, &devs) < 0) {
		perror("Libusb failed to get USB access!");
		std::cin.get();
		return 0;
	}
	
	int i = 0;
	while ((dev = devs[i++]) != NULL) {
		struct libusb_device_descriptor descriptor;
		libusb_get_device_descriptor(dev, &descriptor);
		
		if (descriptor.idVendor == DEV_VID && descriptor.idProduct == DEV_PID) {
			fprintf(stderr, "Found USB ID %04x:%04x\n", descriptor.idVendor, descriptor.idProduct);
			break;
		} else {
			fprintf(stderr, "      USB ID %04x:%04x\n", descriptor.idVendor, descriptor.idProduct);
		}
		dev = NULL;
	}
	
	if (!dev) {
		fprintf (stderr, "device not found!\n");
		std::cin.get();
		return 1;
	}
	
	libusb_get_active_config_descriptor(dev, &active_config);
	libusb_open(dev, &devh);
	
	unsigned char in_endpoint = find_first_bulk_endpoint(active_config, LIBUSB_ENDPOINT_IN, 0);
	unsigned char out_endpoint = find_first_bulk_endpoint(active_config, LIBUSB_ENDPOINT_OUT, 0);
	
	fprintf(stderr, "LIBUSB_ENDPOINT_IN = %d\n", in_endpoint);
	fprintf(stderr, "LIBUSB_ENDPOINT_OUT = %d\n", out_endpoint);
	
	libusb_free_config_descriptor(active_config);
	
	libusb_claim_interface(devh, 0);
	
	const char str[] = "\x55\x53\x42\x43\x12\x34\x56\x78\x24\x00\x00\x00\x80\x01\x08\xdf\x20\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
	usb_bulk_io(devh, out_endpoint, (char *) DEV_MSG, sizeof(DEV_MSG) - 1, 3000);
	
	libusb_clear_halt(devh, in_endpoint);
	libusb_clear_halt(devh, out_endpoint);
	
	usleep(50000);
	
	libusb_release_interface(devh, 0);
	
	libusb_close(devh);
	
	fprintf(stderr, "OK!\n");
	
	std::cin.get();
	
	return 0;
}
