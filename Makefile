OS := $(shell uname)

CC = gcc
LINKER = gcc
ARFLAGS=r
OBJS = libslas.o

ifeq ($(OS),Linux)

    ifeq ($(LIBSLAS_SHARED),1)

        CFLAGS = -fPIC -O -ansi -Wall -c -D_LARGEFILE64_SOURCE -D_XOPEN_SOURCE

        LINK_FLAGS = -shared -fPIC -Wl,-soname,$(TGT) -o $(TGT)

    .c.o:
	$(CC) $(CFLAGS) $*.c

        TGT = libslas.so

all: $(TGT)
{-c $(LINKER) $(LINK_FLAGS)} $(TGT) : $(OBJS) $(MAKEFILE)
	$(LINKER) $(LINK_FLAGS) $(OBJS)

    else

        CFLAGS = -O -ansi -Wall -c -D_LARGEFILE64_SOURCE -D_XOPEN_SOURCE

        .c.o:
	    $(CC) -c $(CFLAGS) $*.c

        TGT = libslas.a

        $(TGT):	$(TGT)($(OBJS))

    endif


else


#   Always build statically on Windows.

    CFLAGS = -ansi -O -Wall -c -D_LARGEFILE64_SOURCE -D_XOPEN_SOURCE -DNVWIN3X

    .c.o:
	$(CC) -c $(CFLAGS) $*.c

    TGT = libslas.a

    $(TGT):	$(TGT)($(OBJS))

endif


libslas.o:  	libslas.h libslas_version.h
