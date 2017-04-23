SRCS := main.c thpool.c error.c
HDRS := thpool.h error.h
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
