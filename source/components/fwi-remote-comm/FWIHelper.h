#ifndef _FWIHELPER_H_
#define _FWIHELPER_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void FWIHELPER_ToHexString(char *dstHexString, const uint8_t* data, uint8_t len);

#ifdef __cplusplus
}
#endif

#endif