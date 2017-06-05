SRCS := main.c thpool.c error.c socks.c silkio.c host_serv.c connect_nonb.c timeo.c tcp_listen.c negotiate.c proxy.c gf_time.c db.c cache.c hashtbl.c
HDRS := thpool.h error.h common.h
OBJS := $(SRCS:.c=.o)
OUT := silksocks
LIBS := -pthread -lodbc
CFLAGS := -DUSEDB -DDEBUG
#CFLAGS := -p -g -O0 -DDEBUG -DUSEDB #-DUSE_SPLICE_NEGOTIATE #-DUSE_SELECT_NEGOTIATE #-DDISABLE_SOCK_TIMEO

all: $(OUT)

tags: *.c *.h
	ctags -R

$(OUT): $(OBJS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LIBS)

clean:
	$(RM) $(OUT) *.o gmon.out
