#ifndef _FWIREMOTECOMM_H_
#define _FWIREMOTECOMM_H_

#include "fwi-output.h"

#define FWIREMOTECOMM_MAGIC 0xFACADE77

#define RESP(_frameID) (_frameID | 0x8000)

typedef enum
{
    FWIREMOTECOMM_EFRAMEID_None = 0,

    FWIREMOTECOMM_EFRAMEID_C2SGetStatus = 1,
    FWIREMOTECOMM_EFRAMEID_S2CGetStatusResp = RESP(FWIREMOTECOMM_EFRAMEID_C2SGetStatus),
    // 
    FWIREMOTECOMM_EFRAMEID_C2SFire = 2,
    FWIREMOTECOMM_EFRAMEID_S2CFireResp = RESP(FWIREMOTECOMM_EFRAMEID_C2SFire),
} FWIREMOTECOMM_EFRAMEID;

typedef struct
{
    uint32_t u32TransactionID;
    uint8_t u8OutputIndex;
} __attribute__((packed)) FWIREMOTECOMM_C2SFire;

typedef struct
{
    uint32_t u32TransactionID;
} __attribute__((packed)) FWIREMOTECOMM_S2CFireResp;

typedef struct
{
    uint32_t u32TransactionID;
} __attribute__((packed)) FWIREMOTECOMM_C2SGetStatus;

typedef struct
{
    uint32_t u32TransactionID;
    uint8_t u8GeneralState;     /* MAINAPP_EGENERALSTATE*/
    uint8_t u8OutState[32];     /* FWIOUTPUT_EOUTPUTSTATE */
} __attribute__((packed)) FWIREMOTECOMM_S2CGetStatusResp;

extern uint32_t g_FWIREMOTECOMM_u32Magic;

#endif