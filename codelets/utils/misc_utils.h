#ifndef CODELET_UTILS_H
#define CODELET_UTILS_H

#include <stdint.h>

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define jbpf_xran_ntohs(x) __builtin_bswap16(x)
#define jbpf_xran_htons(x) __builtin_bswap16(x)
#define jbpf_xran_ntohl(x) __builtin_bswap32(x)
#define jbpf_xran_htonl(x) __builtin_bswap32(x)
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define jbpf_xran_ntohs(x) (x)
#define jbpf_xran_htons(x) (x)
#define jbpf_xran_ntohl(x) (x)
#define jbpf_xran_htonl(x) (x)
#endif

#endif // CODELET_UTILS_H

