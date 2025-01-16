#ifndef DRAMCHANNEL_H
#define DRAMCHANNEL_H

#include "global_types.h"
#include "drambank.h"

#define NUM_DRAM_BUSQ_ENTRIES 128


typedef struct DRAM_Channel   DRAM_Channel;
typedef struct DRAM_BusQ DRAM_BusQ;
typedef struct DRAM_BusQ_Entry DRAM_BusQ_Entry;

typedef struct DRAM_TFAW_Token DRAM_TFAW_Token;
typedef struct DRAM_RDWR_Token DRAM_RDWR_Token;

/////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

struct DRAM_BusQ_Entry{
  Flag valid;
  DRAM_ReqType reqtype;
  uns coreid; // to wake up
  uns robid; // to wake up
  uns64 inst_num; // to wake up
  uns64 ready_time; // when will it be ready for going on bus
};


struct DRAM_BusQ{
  DRAM_BusQ_Entry entries[NUM_DRAM_BUSQ_ENTRIES];
  uns size; //  num valid entries
};


struct DRAM_TFAW_Token {
  uns64     tfaw_time; // TFAW window
  uns64     prev_time[4]; // last 4 activates
};

struct DRAM_RDWR_Token {
  uns64     rdwr_time; // RD to RD delay
  uns64     prev_time; // last operation
};


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

struct DRAM_Channel {
    uns         id; // we will have multiple dram channels, so id helps differentiate
    char name[256];

    Flag       ALERT;

    DRAM_Bank   *bank[32]; // max of 32 banks per channel
    DRAM_BusQ   dbusq; // DRAM bus queue

    DRAM_TFAW_Token  tfaw_token;
    DRAM_RDWR_Token  rdwr_token;

    uns64       num_banks;
    uns64       num_rows;
    uns64       s_ALERT; // stats
    uns64       s_bus_time;
};

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

DRAM_Channel*   dram_channel_new(uns id, uns num_banks, uns num_rows);
void    dram_channel_cycle(DRAM_Channel *c);

uns     dram_channel_insert(DRAM_Channel *c, uns bankid,  DRAM_ReqType type, uns64 rowid, uns coreid, uns robid, uns64 inst_num );

void   dram_channel_insert_rdwrq(DRAM_Channel *c, uns bankid, uns coreid, uns robid, uns64 inst_num, DRAM_ReqType reqtype, uns64 ready_time);
void   dram_channel_schedule_rdwrq(DRAM_Channel *c);


Flag   dram_channel_get_tfaw_token(DRAM_Channel *c, uns64 in_cycle);
Flag   dram_channel_get_rdwr_token(DRAM_Channel *c, uns64 in_cycle);

void   dram_channel_print_state(DRAM_Channel *c);


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


#endif // DRAMCHANNEL_H
