CFLAGS = -lm

# Use everything at your disposal. 
SRCS		=	$(wildcard *.c)
OBJS		=	$(patsubst %.c,%.o,$(SRCS))

all: asz80

asz80: $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS) $(LIBS)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)