# Debug version.
CFLAGS 		=	-g

# Use everything at your disposal. 
SRCS		=	$(wildcard *.c)
OBJS		=	$(patsubst %.c,%.o,$(SRCS))

all: gdb-server-sim

gdb-server-sim: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm *.o