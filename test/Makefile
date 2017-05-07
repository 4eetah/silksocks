CFLAGS := -I../ -DDEBUG=1
OBJS := ../cache.o ../gf_time.o ../error.o ../connect_nonb.o ../host_serv.o
OUT := test_cache test_connect_nonb

all: $(OUT) 

test_cache: test_cache.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

test_connect_nonb: test_connect_nonb.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	$(RM) $(OUT) *.o
