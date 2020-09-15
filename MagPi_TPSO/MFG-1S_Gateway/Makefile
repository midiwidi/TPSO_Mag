#
# 'make depend' uses makedepend to automatically generate dependencies 
#               (dependencies are added to end of Makefile)
# 'make'        build executable file 'MFG-1S_Gateway'
# 'make clean'  removes all .o and executable files
#

CC = arm-linux-gnueabihf-gcc

CFLAGS		= -Wall -g -O0
INCLUDES	= #-I/home/newhall/include  -I../include
LFLAGS		= #-L/home/newhall/lib  -L../lib
LIBS		= #-lmylib -lm

SRCS = MFG-1S_Gateway.c data_processing.c globals.c helpers.c log.c packet.c serial.c signal.c cmd.c fifos.c

OBJS = $(SRCS:.c=.o)

MAIN = MFG-1S_Gateway

.PHONY: depend clean

all:    $(MAIN)
	@echo  MFG-1S_Gateway has been compiled successfully

$(MAIN): $(OBJS) 
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)

depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
