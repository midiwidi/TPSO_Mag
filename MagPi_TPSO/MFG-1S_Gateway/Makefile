#
# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'MFG-1S_Gateway'
# 'make clean'  removes all .o and executable files
#

CC = arm-linux-gnueabihf-gcc

GIT_VERSION := "$(shell git describe --dirty=y --abbrev=7  --always --tags)"
CFLAGS		= -Wall -DVERSION=\"$(GIT_VERSION)\" -D_GNU_SOURCE
ifdef DEBUG
    CFLAGS += -O0 -g
else
	CFLAGS += -O3
endif
INCLUDES	= -I../
LIBS		= ../tomlc99/libtoml.a

SRCS = MFG-1S_Gateway.c data_processing.c globals.c helpers.c log.c packet.c serial.c signal.c cmd.c fifos.c toml.c
DEPS = data_processing.h globals.h helpers.h log.h packet.h serial.h signal.h cmd.h fifos.h toml.h
OBJS = $(SRCS:.c=.o)

MAIN = MFG-1S_Gateway

.PHONY: depend clean

all:    $(MAIN)
	@echo  MFG-1S_Gateway has been compiled successfully

$(MAIN): $(OBJS) $(DEPS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
