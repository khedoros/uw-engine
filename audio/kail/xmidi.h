#define QUANT_RATE 120
#define MAX_NOTES 32
#define FOR_NEST 4
#define NSEQS 8
#define QUANT_ADVANCE 1

#define BRANCH_EXIT true

#include "ail.h"
#include "yamaha.h"
#include<stdint.h>

#define YM3812
#define STEREO
#define NUM_CHANS 16


typedef struct {
    uint8_t PV[NUM_CHANS];
    uint8_t MODUL[NUM_CHANS];
    uint8_t PAN[NUM_CHANS];
    uint8_t EXP[NUM_CHANS];
    uint8_t SUS[NUM_CHANS];
    uint8_t PBS[NUM_CHANS];
    uint8_t C_LOCK[NUM_CHANS];
    uint8_t C_PROT[NUM_CHANS];
    uint8_t V_PROT[NUM_CHANS];
} ctrl_log;


