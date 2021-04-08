CFLAGS =  -ggdb -g -O2 -Wall -Wno-parentheses -DINDEXLIB -DUNIX -I. 
LDFLAGS = -lm 

# Use everything at your disposal. 
SRCS		=	$(wildcard *.c)
OBJS		=	$(patsubst %.c,%.o,$(SRCS))

all: xld

xld: $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

.PHONY: clean
clean:
	rm *.o