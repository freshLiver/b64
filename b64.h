#ifndef __SIMPLE_B64_H__
#define __SIMPLE_B64_H__

#include <unistd.h>

void encode(const char *in, size_t isz, char **out, size_t *osz);
void decode(const char *in, size_t isz, char **out, size_t *osz);

#endif /* __SIMPLE_B64_H__ */
