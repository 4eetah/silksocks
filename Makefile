SRCS := main.c thpool.c error.c socks.c silkio.c host_serv.c connect_nonb.c timeo.c tcp_listen.c negotiate.c proxy.c gf_time.c
HDRS := thpool.h error.h common.h
OBJS := $(SRCS:.c=.o)
OUT := silksocks
LIBS := -pthread
CFLAGS := -O2
#CFLAGS := -g -O0 -DDEBUG

all: $(OUT)

tags: *.c *.h
	ctags -R

$(OUT): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	$(RM) $(OUT) *.o 
