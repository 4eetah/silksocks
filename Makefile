SRCS := main.c thpool.c error.c socks.c
HDRS := thpool.h error.h socks.h
OBJS := $(SRCS:.c=.o)
OUT := silksocks
LIBS := -pthread

all: $(OUT)

tags: *.c *.h
	ctags -R

$(OUT): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	$(RM) $(OUT) $(OBJS)
