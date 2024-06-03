#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "b64.h"

#ifdef DEBUG
#define pr_d(fmt, ...) fprintf(stderr, fmt "\n", ##__VA_ARGS__)
#else
#define pr_d(...)
#endif

int main(int argc, const char *argv[]) {
    int isz = atoi(argv[1]);
    const char *ifile = argc > 2 ? argv[2] : "/dev/random";

    // malloc buffer for
    char *ibuf = calloc(1, isz);
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
    char *obuf;
    encode(ibuf, isz, &obuf, &osz);
    if (obuf == NULL) {
        pr_d("failed to encode data");
        return -1;
    }

    write(STDOUT_FILENO, obuf, osz);

    // do decode
    size_t dsz;
    char *dbuf;
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
