#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libusb.h>
#include <stdio.h>
#include <stdlib.h>
#define VID 0x03f0
#define PID 0xbe2a
#define IFACE 3
#define EP_OUT 0x03
#define EP_IN 0x84

int main(int argc, char **argv) {
	libusb_context *uctx=NULL;
	libusb_init(&uctx);
	libusb_device *ldev;
	ssize_t n;
	int rc;
	/*
	n=libusb_get_device_list(uctx, &ldev);
	printf("n=%d USB devices found\n",n);
	*/
	libusb_device_handle *udh = libusb_open_device_with_vid_pid(uctx, VID, PID);
	if(udh == NULL) {
		printf("Device with vid=%lx pid=%lx not found\n", VID, PID);
		exit(1);
	}
	if((rc=libusb_set_auto_detach_kernel_driver(udh, 1))!=0) {
		printf("libusb_set_auto_detach failed err=%s\n", libusb_error_name(rc));
		exit(1);
	}
	int config;
	if((rc=libusb_get_configuration(udh, &config))!=0) {
		printf("libusb_get_configuration failed err=%s\n", libusb_error_name(rc));
		exit(1);
	}
	printf("config=%d\n", config);
	if((rc=libusb_claim_interface(udh, 3))!=0) {
		printf("libusb_claim_interface failed err=%s\n", libusb_error_name(rc));
		exit(1);
	}
	char req[512];
	char data[512];
	int transferred;

	while((rc=libusb_bulk_transfer(udh, EP_IN, data, 512, &transferred, 1000))==0) {
		printf("discarding %d bytes\n", transferred);
		if(transferred==0) break;
	}

	if(argc<3) { printf("Usage: %s method url\n", argv[0]); }

	sprintf(req,"%s %s HTTP/1.1\r\nHost: localhost\r\n\r\n",argv[1],argv[2]);
	rc=libusb_bulk_transfer(udh, EP_OUT, req, strlen(req), &transferred, 0);
	if(rc<0) {
		printf("libusb_bulk_transfer(OUT) err=%s\n", libusb_error_name(rc));
		exit(1);
	}
	do {
			rc=libusb_bulk_transfer(udh, EP_IN, data, 512, &transferred, 0);
			usleep(100);
			write(0, data, transferred);
	} while(1);
}

