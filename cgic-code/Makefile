CFLAGS=-g -fPIC -EL -c -O1 -W -ffunction-sections -fdata-sections -mips32r2 -mdspr2 -mno-branch-likely
CC=mips-linux-gnu-gcc
AR=ar
RANLIB=mips-linux-gnu-ranlib
LIBS=-L./ -lcgic

all: libcgic.a cgictest.cgi capture pwctrl_step1

install: libcgic.a
	cp libcgic.a /usr/local/lib
	cp cgic.h /usr/local/include
	@echo libcgic.a is in /usr/local/lib. cgic.h is in /usr/local/include.

libcgic.a: cgic.o cgic.h
	rm -f libcgic.a
	$(AR) rc libcgic.a cgic.o
	$(RANLIB) libcgic.a

#mingw32 and cygwin users: replace .cgi with .exe

cgictest.cgi: cgictest.o libcgic.a
	mips-linux-gnu-gcc  -g -rdynamic -EL -O1 -gc-sections -mips32r2  -L. cgictest.o -o cgictest.cgi ${LIBS}

capture: capture.o libcgic.a
	mips-linux-gnu-gcc  -g -rdynamic -EL -O1 -gc-sections -mips32r2  -L. capture.o -o capture ${LIBS}

pwctrl_step1: pwctrl_step1.o libcgic.a
	mips-linux-gnu-gcc  -g -rdynamic -EL -O1 -gc-sections -mips32r2  -L. pwctrl_step1.o -o pwctrl_step1 ${LIBS}

clean:
	rm -f *.o *.a cgictest.cgi capture pwctrl_step1

