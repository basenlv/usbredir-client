
CFLAGS=
CPPFLAGS= -I/usr/include/libusb-1.0
LDFLAGS= 
LIBS= -lusbredir -lusb-1.0 -lusbredirhost -lusbredirparser
OFILE=usbredir_mgr.o usbredir_om.o usbredir_event.o \
          usbredir_log.o usbredir_monitor.o usbredir_init.o

all:
	gcc $(CPPFLAGS) -Wall -fPIC -c *.c
	gcc -shared -Wl,-soname,libusbredir.so -o libusbredir.so.1.0 $(OFILE)
	gcc $(CPPFLAGS) -o usbredir_test usbredir_test.c $(LIBS)

install:
	cp -f libusbredir.so.1.0 /usr/lib/ 
	ln -sf /usr/lib/libusbredir.so.1.0 /usr/lib/libusbredir.so.1
	ln -sf /usr/lib/libusbredir.so.1.0 /usr/lib/libusbredir.so
	cp -f usbredir_test /usr/bin/ 
	cp -f usbredir_rules.conf /etc/

test:
	gcc $(CPPFLAGS) -o usbredir_test usbredir_test.c $(LIBS)

test_install:
	cp usbredir_test /usr/bin/ -f

clean:
	rm *.o
	rm *.so.*
	rm usbredir_test

