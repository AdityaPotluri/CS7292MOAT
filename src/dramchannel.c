#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "mcore.h"
#include "externs.h"
#include "dramchannel.h"

extern MCore *mcore[MAX_THREADS];

extern uns64 cycle;

uns64 tFAW=12*4;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

DRAM_Channel*   dram_channel_new(uns id, uns num_banks, uns num_rows){
  DRAM_Channel *c = (DRAM_Channel *) calloc (1, sizeof (DRAM_Channel));
  c->id = id;
  c->num_banks = num_banks;
  c->num_rows = num_rows;

  for(uns ii=0; ii< num_banks; ii++){
    c->bank[ii] = dram_bank_new(ii, id, num_rows/num_banks);
  }

  c->tfaw_token.tfaw_time = tFAW;
  c->rdwr_token.rdwr_time = tRDRD;
  return c;
}


////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

void dram_channel_alert(DRAM_Channel *c){
  uns ii;
  uns64 max_bank_ready_cycle = cycle;

  for(ii=0; ii < c->num_banks; ii++){
    if(c->bank[ii]->sleep_cycle > max_bank_ready_cycle){
      max_bank_ready_cycle = c->bank[ii]->sleep_cycle;
    }
  }

  for(ii=0; ii < c->num_banks; ii++){
    dram_bank_alert(c->bank[ii],max_bank_ready_cycle); // no bank-q
  }

}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

void  dram_channel_cycle(DRAM_Channel *c){
  uns ii;

  dram_channel_schedule_rdwrq(c);

  for(ii=0; ii<c->num_banks; ii++){
    dram_bank_cycle(c->bank[ii]);
  }


  if(c->ALERT){
    c->ALERT=FALSE;
    c->s_ALERT++;
    dram_channel_alert(c);
  }
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

uns   dram_channel_insert(DRAM_Channel *c, uns bankid,  DRAM_ReqType type,
			  uns64 rowid, uns coreid, uns robid, uns64 inst_num ){

  assert(bankid < c->num_banks);
  return dram_bank_insert(c->bank[bankid],type, rowid, coreid, robid, inst_num);
}


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void  dram_channel_insert_rdwrq(DRAM_Channel *c, uns bankid, uns coreid, uns robid, uns64 inst_num, DRAM_ReqType reqtype, uns64 ready_time){

  assert(c->dbusq.entries[bankid].valid == FALSE);

  c->dbusq.entries[bankid].valid = TRUE;
  c->dbusq.entries[bankid].coreid = coreid;
  c->dbusq.entries[bankid].robid = robid;
  c->dbusq.entries[bankid].inst_num = inst_num;
  c->dbusq.entries[bankid].reqtype = reqtype;
  c->dbusq.entries[bankid].ready_time = ready_time;
  c->dbusq.size++;

}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void   dram_channel_schedule_rdwrq(DRAM_Channel *c){

  uns offset = cycle%c->num_banks; // avoid starvation, start roundrobin

   for(uns ii=0; ii<c->num_banks; ii++){
     uns index = (ii+offset)%c->num_banks;
     
    if(c->dbusq.entries[index].valid == TRUE){
      if(cycle >= c->dbusq.entries[index].ready_time){
	if(dram_channel_get_rdwr_token(c, cycle)){
	  uns64 rdwr_bus_delay = tCAS+tRDRD;
	 
	  if(c->dbusq.entries[index].reqtype == DRAM_REQ_RD){
	     uns mycoreid = c->dbusq.entries[index].coreid;
	     uns myrobid = c->dbusq.entries[index].robid;
	     uns64 myinstnum = c->dbusq.entries[index].inst_num;

	     if(mycoreid == DEBUG_CORE_ID){
	       printf("CHANNEL-RDWR: ChannelID: %u CoreID: %u RobID: %u  InstNum: %llu Type: %u Cycle: %llu\n",
		      c->id, mycoreid, myrobid, myinstnum, (uns) c->dbusq.entries[index].reqtype, cycle);
	     }
	     
	     if(myrobid >= ROB_SIZE){
	       printf("MyCoreID: %u MyRobID: %u InstNum: %llu\n", mycoreid, myrobid, myinstnum);
	     }
	     mcore_rob_wakeup(mcore[mycoreid], myrobid, myinstnum);
	  }
	  c->s_bus_time += rdwr_bus_delay;
	  c->dbusq.size--;
	  c->dbusq.entries[index].valid = FALSE;
	  c->bank[index]->sleep_cycle = (cycle+tRDRD); // bank busy ...
	}
	return; // if checked token, then no more token left
      }
    }
  }

}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////


Flag   dram_channel_get_tfaw_token(DRAM_Channel *c, uns64 in_cycle){
  if( (in_cycle -  c->tfaw_token.prev_time[0]) >= c->tfaw_token.tfaw_time){
    c->tfaw_token.prev_time[0] = c->tfaw_token.prev_time[1];
    c->tfaw_token.prev_time[1] = c->tfaw_token.prev_time[2];
    c->tfaw_token.prev_time[2] = c->tfaw_token.prev_time[3];
    c->tfaw_token.prev_time[3] = in_cycle;
    return TRUE;
  }

  return FALSE;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

Flag   dram_channel_get_rdwr_token(DRAM_Channel *c, uns64 in_cycle){
  if( (in_cycle -  c->rdwr_token.prev_time) >= c->rdwr_token.rdwr_time){
    c->rdwr_token.prev_time = in_cycle;
    return TRUE;
  }
  return FALSE;
}

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void   dram_channel_print_state(DRAM_Channel *c){
  printf("\nCh: %u\t", c->id);
  printf("BusQ: %2u\t", c->dbusq.size);
  for(uns ii=0; ii< c->num_banks; ii++){
    printf("%2u ",c->bank[ii]->bankq.size);
  }
}

///////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////
