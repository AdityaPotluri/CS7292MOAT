#ifndef __EXTERNS_H__
#define __EXTERNS_H__

#include "global_types.h"

extern void die_message(const char *msg);

extern uns64       USE_IMAT_TRACES;
extern uns64       INST_LIMIT;
extern uns64       LINESIZE;     
extern uns64       OS_PAGESIZE; 
extern uns64       OS_NUM_RND_TRIES;
extern uns64       NUM_THREADS;

extern uns64       CORE_WIDTH;
extern uns64       ROB_SIZE;


extern uns64       L3_LATENCY;
extern uns64       L3_PERFECT;

extern uns64       MEM_SIZE_MB;
extern uns64       MEM_RSRV_MB;
extern uns64       MEM_CHANNELS;
extern uns64       MEM_BANKS;   
extern uns64       MEM_PAGESIZE;
extern uns64       MEM_CLOSEPAGE;

extern uns64       DRAM_REF_POLICY;
extern uns64       DRAM_SCHED_POLICY;
extern uns64       DRAM_MAP_POLICY;
extern uns64       DRAM_MOP_GANGSIZE;
extern uns64       DRAM_MAX_TOPEN;
extern uns64       DRAM_RFM_TH;
extern uns64       ENABLE_MOAT;
extern uns64       REFS_PER_MITIG;
extern uns64       RESET_CTR_ON_REF;
extern uns64       MOAT_ATH;
extern uns64       MOAT_ETH;
extern uns64       MOAT_LEVEL;

extern uns64       tRC;
extern uns64       tRFC;
extern uns64       tACT;
extern uns64       tCAS;
extern uns64       tPRE;
extern uns64       tRAS;
extern uns64       tRDRD;
extern uns64       tREFI;
extern uns64       tREFW;
extern uns64       tBUS;
extern uns64       tALERT;


// these are non-param global variables
extern int         num_threads;

#endif


