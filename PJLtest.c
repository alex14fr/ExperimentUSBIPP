#include <fcntl.h>
#include <string.h>
#include <unistd.h>

void writePjlCmd(int fd, char *s) {
	char pjlStart[]="\e%-12345X@PJL ";
	char pjlEnd[]="\r\n\e%-12345X";
	write(fd,pjlStart,strlen(pjlStart));
	write(fd,s,strlen(s));
	write(fd,pjlEnd,strlen(pjlEnd));

	printf("--> @PJL %s\n", s);
}

void main(int argc, char **argv) {
	int fd;
	fd=open(argv[1],O_RDWR);
	printf("fd=%d\n",fd);

	writePjlCmd(fd,argv[2]);	

	char c;
	while(1) {
		while(read(fd,&c,1)>0) {
			putchar(c);
		}
	}

}
