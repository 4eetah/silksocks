#include <stdint.h>

#define BUFSIZE 512
#define PACKED __attribute__((packed))

#define SOCKS4_VER 0x04

#define SOCKS5_VER 0x05
#define SOCKS5_AUTH_VER 0x01
#define SOCKS5_AUTH_NONE 0x00
#define SOCKS5_AUTH_GSSAPI 0x01
#define SOCKS5_AUTH_PASSWD 0x02
#define SOCKS5_AUTH_IANA_ASSIGNED 0x03
#define SOCKS5_AUTH_RESERVED 0x80
#define SOCKS5_AUTH_INVALID 0xFF

void do_socks4(int fd);
void do_socks5(int fd);

typedef struct socks4_method_req_t {
    uint8_t dummy[0];
} PACKED socks4_method_req;

typedef struct socks4_method_reply_t {
    uint8_t dummy[0];
} PACKED socks4_method_reply;

typedef struct socks5_method_req_t {
    uint8_t ver;
    uint8_t nr_methods;
    uint8_t methods[255];
} PACKED socks5_method_req;

typedef struct socks5_method_reply_t {
    uint8_t ver;
    uint8_t method;
} PACKED socks5_method_reply;
