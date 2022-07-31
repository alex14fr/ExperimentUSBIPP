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
	int sockfd;
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&(int){1},sizeof(int));
	struct sockaddr_in saddr;
	saddr.sin_family=AF_INET;
	saddr.sin_port=htons(6332);
	saddr.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
	if(bind(sockfd,(struct sockaddr*)&saddr,sizeof(struct sockaddr_in))<0) { perror("bind"); exit(1); }
	int s2;
	char req[4096];
	char data[512];
	int transferred;
	if(listen(sockfd,1)<0) { perror("listen"); exit(1); }

	while((rc=libusb_bulk_transfer(udh, EP_IN, data, 512, &transferred, 1000))==0) {
		printf("discarding %d bytes\n", transferred);
		if(transferred<512) break;
	}

	while(s2=accept(sockfd,NULL,0)) {
		int nToTransfer;
		int fl=fcntl(s2, F_GETFL, NULL);
		fl |= O_NONBLOCK;
		fcntl(s2, F_SETFL, fl);
		while(1) {
			if((nToTransfer=read(s2, data, 512))>0) {
				write(STDOUT_FILENO, data, nToTransfer);
				int nOK=0;
				do {
					rc=libusb_bulk_transfer(udh, EP_OUT, data+nOK, nToTransfer-nOK, &transferred, 0);
					if(rc<0) {
						printf("libusb_bulk_transfer(OUT) err=%s\n", libusb_error_name(rc));
						exit(1);
					}
					nOK+=transferred;
				} while(nOK<nToTransfer);
			} 
			if(nToTransfer==0) {
				printf("connection closed\n");
				break;
			}
			rc=libusb_bulk_transfer(udh, EP_IN, data, 512, &nToTransfer, 0);
			if(rc<0) {
				printf("libusb_bulk_transfer(IN) err=%s\n", libusb_error_name(rc));
				exit(1);
			}
			if(nToTransfer>0) {
				int nOK=0;
				write(STDOUT_FILENO, data, nToTransfer);
				do {
					transferred=write(s2, data+nOK, nToTransfer-nOK);
					nOK+=transferred;
				} while(nOK<nToTransfer);
			}
			usleep(50);
		}
	}
}

