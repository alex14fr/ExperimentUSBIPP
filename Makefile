CFLAGS+=$(shell pkg-config --cflags libusb-1.0)
LIBS+=$(shell pkg-config --libs libusb-1.0)
.c:
	$(CC) -o $@ $(CFLAGS) $< $(LIBS)

