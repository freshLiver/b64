#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define pr(fmt, ...) fprintf(stdout, fmt "\n", ##__VA_ARGS__)

#ifdef DEBUG
#define pr_d(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#else
#define pr_d(...)
#endif

static const char B64MAP[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

static const uint8_t B64UNMAP[256] = {
    ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,  ['E'] = 4,  ['F'] = 5,
    ['G'] = 6,  ['H'] = 7,  ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11,
    ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15, ['Q'] = 16, ['R'] = 17,
    ['S'] = 18, ['T'] = 19, ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
    ['Y'] = 24, ['Z'] = 25, ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29,
    ['e'] = 30, ['f'] = 31, ['g'] = 32, ['h'] = 33, ['i'] = 34, ['j'] = 35,
    ['k'] = 36, ['l'] = 37, ['m'] = 38, ['n'] = 39, ['o'] = 40, ['p'] = 41,
    ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45, ['u'] = 46, ['v'] = 47,
    ['w'] = 48, ['x'] = 49, ['y'] = 50, ['z'] = 51, ['0'] = 52, ['1'] = 53,
    ['2'] = 54, ['3'] = 55, ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59,
    ['8'] = 60, ['9'] = 61, ['+'] = 62, ['/'] = 63};

typedef union {
    uint32_t in32;
    uint8_t in8[4];
} B64_t;

void encode(const char* in, size_t isz, char** out, size_t* osz) {
    int trailing = isz % 3;
    int nPad = trailing ? 3 - trailing : 0;
    size_t groups = isz / 3 + !!trailing;
    pr_d("G: %lu + T(P): %d(%d)", groups, trailing, nPad);

    // allocate output buffer
    *osz = (isz + nPad) / 3 * 4;
    *out = calloc(1, *osz);
    if (!*out) return;

    // convert every 3 in chars to 4 out chars
    B64_t input;
    for (size_t g = 0, io = 0, oo = 0; g < groups; ++g, io += 3, oo += 4) {
        input.in8[0] = in[io + 2];
        input.in8[1] = in[io + 1];
        input.in8[2] = in[io];
        input.in8[3] = 0;
        // in32 = ((u32)in[io] << 16) | ((u32)in[io + 1] << 8) | in[io + 2];

        (*out)[oo + 0] = B64MAP[((input.in32 & 0xFC0000) >> 18) & 0x3F];
        (*out)[oo + 1] = B64MAP[((input.in32 & 0x3F000) >> 12) & 0x3F];
        (*out)[oo + 2] = B64MAP[((input.in32 & 0xFC0) >> 6) & 0x3F];
        (*out)[oo + 3] = B64MAP[input.in32 & 0x3F];
    }

    // may need to pad '=' to output
    for (int iPad = 0; iPad < nPad; ++iPad) (*out)[*osz - 1 - iPad] = '=';
}

void decode(const char* in, size_t isz, char** out, size_t* osz) {
    size_t groups = isz / 4;

    // alloc output buffer
    *osz = isz / 4 * 3;
    *out = calloc(1, *osz);
    if (!*out) return;

    // decode every 4 char to 3 char
    B64_t input;
    for (size_t g = 0, io = 0, oo = 0; g < groups; ++g, io += 4, oo += 3) {
        input.in32 = (B64UNMAP[in[io + 0]] & 0x3F) << 18;
        input.in32 |= (B64UNMAP[in[io + 1]] & 0x3F) << 12;
        input.in32 |= (B64UNMAP[in[io + 2]] & 0x3F) << 6;
        input.in32 |= (B64UNMAP[in[io + 3]] & 0x3F);

        (*out)[oo + 2] = input.in8[0];
        (*out)[oo + 1] = input.in8[1];
        (*out)[oo + 0] = input.in8[2];
    }

    // last 2 char may be padding
    *osz -= !!(in[isz - 1] == '=');
    *osz -= !!(in[isz - 2] == '=');
}

int main(int argc, const char* argv[]) {
    int isz = atoi(argv[1]);
    const char* ifile = argc > 2 ? argv[2] : "/dev/random";

    // malloc buffer for
    char* ibuf = calloc(1, isz);
    if (!ibuf) {
        pr_d("failed to calloc input buffer!");
        return errno;
    }

    // read data from specified (or default) file
    pr_d("read %d bytes from %s", isz, ifile);
    int fd = open(ifile, O_RDONLY);
    int ret = read(fd, ibuf, isz);
    if (ret != isz) {
        pr_d("expect read %d, but %d", isz, ret);
        free(ibuf);
        return ret;
    }
    pr_d("%s", ibuf);

    // do encode
    size_t osz;
    char* obuf;
    encode(ibuf, isz, &obuf, &osz);
    if (obuf == NULL) {
        pr_d("failed to encode data");
        return -1;
    }

    // pr("%s", obuf);

    // do decode
    size_t dsz;
    char* dbuf;
    decode(obuf, osz, &dbuf, &dsz);
    if (!dbuf) {
        pr_d("failed to decode data");
        return -1;
    }
    // pr("%s", dbuf);

    // do verify
    assert(isz == dsz);
    assert(!memcmp(ibuf, dbuf, isz));

    free(obuf);
    return 0;
}
