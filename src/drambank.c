#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "mcore.h"
#include "externs.h"
#include "drambank.h"
#include "dramchannel.h"
#include "dram.h"
#include "memsys.h"


uns64 RFMTH=0;
uns64 RFMREFTH=0;
uns64 RAAMMT=6*72;


#define BANK_VERBOSE 1

extern MCore *mcore[MAX_THREADS];
extern MemSys *memsys;

extern uns64 cycle;

////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

DRAM_Bank* dram_bank_new(uns bankid, uns channelid, uns num_rows)
{
  DRAM_Bank *b = (DRAM_Bank *) calloc (1, sizeof (DRAM_Bank));
  b->num_rows  = num_rows;
  b->id        = bankid;
  b->channelid = channelid;

  
  if(MEM_CLOSEPAGE)
  {
    DRAM_MAX_TOPEN = tRAS; // closed page policy closes page after t_RAS
  }

  // [TASK A] Allocate and Initialize the PRAC Counters
  b->PRAC = (uns *) calloc(num_rows, sizeof(uns));

  // TODO: [TASK B] Initialize the MOAT queue
  for (int i = 0; i < MAX_MOAT_LEVEL; i++)
  {
    b->moat_queue[i] = -1;
  }
  
  return b;
}


////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

void dram_bank_cycle(DRAM_Bank *b)
{
  // if bank is busy check if it is free 
  if(b->status == DRAM_BANK_BUSY)
  {
    if(cycle > b->sleep_cycle)
    {
      b->status = DRAM_BANK_READY;
    }
    else
    {
      return;
    }
  }

  // if queue is empty go away
  if(b->bankq.size == 0)
  {
    return;
  }

  // if the bank is waiting for RDWR token, go away
  DRAM_Channel *mychannel = memsys->mainmem->channel[b->channelid];
  if(mychannel->dbusq.entries[b->id].valid)
  {
    return;
  }

  
  // call schedule to pick an entry, call service -- take time, change q-entry status
  uns index = dram_bank_schedule(b);
  assert(b->bankq.entries[index].status == DRAM_BANKQ_ENTRY_WAIT);
  uns64 bank_delay = dram_bank_service(b,  b->bankq.entries[index].reqtype, b->bankq.entries[index].rowid);

  b->bankq.entries[index].status = DRAM_BANKQ_ENTRY_INSERVICE;
  uns64 tot_delay = bank_delay; 
  b->bankq.entries[index].done_time = cycle + tot_delay;
  b->bankq.entries[index].service_delay =  tot_delay;
  uns64 q_wait_time = cycle -  b->bankq.entries[index].birth_time;
  b->bankq.entries[index].wait_delay =  q_wait_time;

  //--- instant model with bus schedule pushed to channel--
  b->bankq.entries[index].status = DRAM_BANKQ_ENTRY_DONE;
  
  if( (b->bankq.entries[index].reqtype == DRAM_REQ_RD)||(b->bankq.entries[index].reqtype == DRAM_REQ_WB)){
    uns mycoreid = b->bankq.entries[index].coreid;
    uns myrobid = b->bankq.entries[index].robid;
    uns64 myinstnum = b->bankq.entries[index].inst_num;
    uns64 myreadytime = b->bankq.entries[index].done_time;
    DRAM_ReqType myreqtype = b->bankq.entries[index].reqtype;

    if(BANK_VERBOSE && mycoreid == DEBUG_CORE_ID)
    {
      printf("BANK-TRANSFER: ChannelID: %u BankID: %u CoreID: %u RobID: %u  InstNum: %llu Type: %u Cycle: %llu\n",
	      b->channelid, b->id, mycoreid, myrobid, myinstnum, (uns) myreqtype, cycle);
    }
    
    dram_channel_insert_rdwrq(mychannel, b->id, mycoreid, myrobid, myinstnum, myreqtype, myreadytime); // RD-WR Q
    b->s_service_delay += bank_delay ;
    b->s_wait_delay += b->bankq.entries[index].wait_delay ;
  }
    
  //remove
  dram_bank_remove(b, index);

}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

uns dram_bank_insert(DRAM_Bank *b,  DRAM_ReqType type, uns64 rowid, uns coreid, uns robid, uns64 inst_num )
{

  assert(rowid < b->num_rows);
 
  if(b->bankq.size == NUM_DRAM_BANK_ENTRIES)
  {
    return 0;
  }

  if( (type==DRAM_REQ_RD)|| (type==DRAM_REQ_WB))
  {
    b->s_REQ++;
  }

  uns index = (b->bankq.ptr + b->bankq.size)%NUM_DRAM_BANK_ENTRIES ;

  if( b->bankq.entries[index].status != DRAM_BANKQ_ENTRY_INVALID){
    printf("Cycle: %llu ChannelID: %u BankID: %u Bank Ptr: %u  Size: %u Status: %u Type: %u\n",
	   cycle, b->id, b->channelid, b->bankq.ptr, b->bankq.size,
	   (uns)b->bankq.entries[index].status, (uns)b->bankq.entries[index].reqtype );
  }

  assert( b->bankq.entries[index].status == DRAM_BANKQ_ENTRY_INVALID);
  b->bankq.entries[index].reqtype = type;
  b->bankq.entries[index].rowid = rowid;
  b->bankq.entries[index].coreid = coreid;
  b->bankq.entries[index].robid = robid;
  b->bankq.entries[index].inst_num = inst_num;
  b->bankq.entries[index].status = DRAM_BANKQ_ENTRY_WAIT;
  b->bankq.entries[index].birth_time = cycle;
  
  if(BANK_VERBOSE && coreid == DEBUG_CORE_ID)
  {
     printf("BANK-INSERT: ChannelID: %u BankID: %u CoreID: %u RobID: %u InstNum: %llu Type: %u Cycle: %llu\n",
	    b->channelid, b->id, coreid, robid, inst_num, (uns) type, cycle);
  }


  b->bankq.size++;

  return 1; // success
}

///////////////////////////////////////////////////////////
// if qentry is done, remove from q, wake up ROB entry (PTR is not needed, as we shift)
////////////////////////////////////////////////////////////

void dram_bank_remove(DRAM_Bank *b,  uns index){
  assert( b->bankq.size > 0);
  assert( b->bankq.entries[index].status == DRAM_BANKQ_ENTRY_DONE);

  b->bankq.entries[index].status = DRAM_BANKQ_ENTRY_INVALID;

  //--- special casing FCFS (oldest selected always)
  if(index == b->bankq.ptr){
    b->bankq.ptr =  (b->bankq.ptr+1)% NUM_DRAM_BANK_ENTRIES;
    b->bankq.size--;
    return;
  }

  //-- else we need to move all entries past the index 
  uns crossover_done=0;
  
  for(uns ii=0; ii< b->bankq.size; ii++){
    if( (b->bankq.ptr + ii)%NUM_DRAM_BANK_ENTRIES == index){
      crossover_done=1;
    }
    if(crossover_done){
      uns curr = (b->bankq.ptr + ii)%NUM_DRAM_BANK_ENTRIES;
      uns next = (curr+1)%NUM_DRAM_BANK_ENTRIES;
      b->bankq.entries[curr] = b->bankq.entries[next];
    }
  }

  uns last = (b->bankq.ptr + b->bankq.size)%NUM_DRAM_BANK_ENTRIES;
  b->bankq.entries[last].status = DRAM_BANKQ_ENTRY_INVALID;
  b->bankq.size--;
}

////////////////////////////////////////////////////////////
// sleeps, returns delay for done 
////////////////////////////////////////////////////////////

/*OAT-RP converts the row-open time (tON) into an equiva-
lent number of activations [35]. The bank measures tON and
divides it into epochs of duration tEPOCH. We use a tEPOCH
of 180ns. If each Rowhammer activation incurs a charge loss
of 1 unit, keeping the row open for up to 180 ns incurs a
charge loss of 1.5 units [ 26 ]. We call this Damage-Per-Epoch
(DPE). A pattern keeping the row-open for up-to 180ns in-
crements the PRAC counter by 1, for up-to 360ns by 2, and
up-to 3490ns (tREFI-tRFC) by 20. Then, MOAT can obtain
the effective damage as PRAC counter times DPE (1.5x). */

uns64 dram_bank_service(DRAM_Bank *b,  DRAM_ReqType type, uns64 rowid)
{
  uns64 retval=0;
  Flag new_act=FALSE; // did we do a new activation?

  assert(b->status != DRAM_BANK_BUSY);

  // if type == N, figure out the delay, sleep time, rowbuffer staus
  if( (type == DRAM_REQ_RD) || (type == DRAM_REQ_WB)){
    uns64 act_delay=0;

    //check for early page closure
    if(b->row_valid)
    {
        if(cycle >= b->rowbufclose_cycle)
        {
            // This is where precharge happens - update PRAC counter here for MOAT-RP
            if(ENABLE_RP)
            {
                uns64 tON = cycle - b->rowbufopen_cycle;
                // Convert cycles to ns (assuming 4 cycles = 1ns)
                tON = tON / 4;
                
                // Calculate PRAC increment based on tON duration
                uns increment = 0;
                if(tON <= 180) {  // up to 180ns
                    increment = 1;
                } else if(tON <= 360) {  // up to 360ns
                    increment = 2;
                } else {  // up to tREFI-tRFC (3490ns)
                    increment = 20;  // Maximum increment for long open times
                }
                
                b->PRAC[b->open_row_id] += increment;

                // Only check for alert, don't do immediate mitigation
                if (ENABLE_MOAT && b->PRAC[b->open_row_id] >= MOAT_ATH) {
                    memsys->mainmem->channel[b->channelid]->ALERT = TRUE;
                }
            }
            
            b->row_valid = FALSE;
            uns64 delta = cycle - b->rowbufclose_cycle;
            if(delta < tPRE)
            {
                act_delay += (tPRE-delta);
            }
        }
    }

    // empty
    if(b->row_valid == FALSE)
    {
      act_delay+=tACT;
      b->rowbufopen_cycle = cycle;
      b->rowbufclose_cycle = b->rowbufopen_cycle+cycle+DRAM_MAX_TOPEN; // page closure
      b->row_valid = TRUE;
      b->open_row_id = rowid;
      new_act = TRUE;
    }
    else
    {
      act_delay=0; // hit

      // conflict
      if(b->open_row_id != rowid)
      {
        uns64 ras_delay=0, pre_delay=tPRE;
        uns64 delta = cycle - b->rowbufopen_cycle;
        if(delta < tRAS){
          ras_delay = tRAS-delta;
        }
        act_delay += (ras_delay + pre_delay +tACT);
        b->rowbufopen_cycle = cycle + ras_delay + tPRE ;
        b->rowbufclose_cycle = b->rowbufopen_cycle+DRAM_MAX_TOPEN; // page closure
        b->row_valid = TRUE;
        b->open_row_id = rowid;
        new_act=TRUE;
      }
    }

    retval = act_delay;
    b->status = DRAM_BANK_BUSY;
    b->sleep_cycle = cycle + act_delay + tRDRD;
  }

  if(type == DRAM_REQ_NRR)
  {
    b->status = DRAM_BANK_BUSY;
    b->sleep_cycle = cycle + (2*tRC);
    b->row_valid = FALSE;
  }

  if(type == DRAM_REQ_REF)
  {
    dram_bank_refresh(b,cycle);
  }

  //---- if ACT done, update states, PRAC, MOAT -------
  if(new_act)
  {
    b->s_ACT++; // counted only for RD and WR
    
    // Always increment PRAC for activations
    b->PRAC[rowid]++;
    
    if(ENABLE_MOAT)
    {
        // Only check for alert, don't do immediate mitigation
        if(b->PRAC[rowid] >= MOAT_ATH) {
            memsys->mainmem->channel[b->channelid]->ALERT = TRUE;
        }
        dram_moat_check_insert(b, rowid);
    }
  }

  return retval;
}


////////////////////////////////////////////////////////////
//  return the index of the bankq entry selected for schedule
////////////////////////////////////////////////////////////

uns dram_bank_schedule(DRAM_Bank *b){
  assert(b->bankq.size);

  if(DRAM_SCHED_POLICY == DRAM_SCHED_FCFS){
    return b->bankq.ptr;
  }

  if(DRAM_SCHED_POLICY == DRAM_SCHED_FRFCFS)
  {
    if(b->row_valid)
    {
      for(uns ii=0; ii< b->bankq.size; ii++)
      {
        uns index = (b->bankq.ptr + ii)%NUM_DRAM_BANK_ENTRIES ;
        if( (b->bankq.entries[index].reqtype == DRAM_REQ_RD) &&
            (b->bankq.entries[index].rowid == b->open_row_id))
        {
          return index;
        }
      }
    }
    return b->bankq.ptr;
  }

  assert(0);
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

void dram_bank_refresh(DRAM_Bank *b, uns64 in_cycle){
    b->status = DRAM_BANK_BUSY;
    b->sleep_cycle = in_cycle + tRFC;
    b->row_valid = FALSE;

    // TODO: [Task A] Reset the PRAC counters, remember the rows are refreshed in 8192 chunks
    if(RESET_CTR_ON_REF)
    {
      for (int i = 0; i < 8; i++)
      {
        int row_ind = ((b->ref_ptr) * 8) + i;
        b->PRAC[row_ind] = 0; 
      }
    }

	
    b->ref_ptr = (b->ref_ptr+1) % 8192;

    // [Task B] Issue a MOAT mitigation
    if(REFS_PER_MITIG && b->ref_ptr % REFS_PER_MITIG == 0)
    {
      if(ENABLE_MOAT)
      {
	      dram_moat_mitig(b);
      }
    }    
}


////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

void  dram_bank_alert(DRAM_Bank *b, uns64 in_cycle)
{
  b->status = DRAM_BANK_BUSY;
  b->sleep_cycle = in_cycle + tALERT;
  b->row_valid = FALSE;

  assert(ENABLE_MOAT);

  //-- as many mitigs as level
  for(uns ii=0; ii < MOAT_LEVEL; ii++)
  {
    dram_moat_mitig(b);
  }
}


////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// TODO: [Task B] Implement dram_moat_check_insert
// Check if the rowid has exceeded the MOAT_ATH and MOAT_ETH thresholds
// If the rowid has exceeded the MOAT_ATH threshold, set the ALERT flag in the channel
//     - memsys->mainmem->channel[b->channelid]->ALERT = TRUE;
// If the rowid is not already present in the MOAT queue, insert it
// If the MOAT queue is full, replace the rowid with the minimum PRAC value in the MOAT queue
void dram_moat_check_insert(DRAM_Bank *b, uns rowid)
{
    // If PRAC value exceeds the alert threshold, raise alert
    if (b->PRAC[rowid] >= MOAT_ATH) {
        memsys->mainmem->channel[b->channelid]->ALERT = TRUE;
        // Don't do immediate mitigation here - let the regular mitigation handle it
    }

    // Only proceed if PRAC value exceeds entry threshold
    if (b->PRAC[rowid] <= MOAT_ETH) {
        return;
    }

    // Check if row is already in MOAT queue
    if (b->moat_queue[0] == (int)rowid) {
        return;  // Row already being tracked
    }

    // If queue is empty or new row has higher PRAC value, insert it
    if (b->moat_queue[0] == -1 || b->PRAC[rowid] > b->PRAC[b->moat_queue[0]]) {
        b->moat_queue[0] = rowid;
    }
}


////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
// TODO: [Task B] Implement dram_moat_mitig
// Check if there are any valid rows in the MOAT queue
// If there are no valid rows, return
// Find the row with the maximum PRAC value in the MOAT queue
// Set the PRAC value of the row to 0
// Reset the MOAT queue entry
// Increment the s_mitigs counter
// Make sure to increment the PRAC values of the neighboring rows
void dram_moat_mitig(DRAM_Bank *b)
{
    // Return if queue is empty
    if (b->moat_queue[0] == -1) return;

    uns victim_row = b->moat_queue[0];
    
    // Reset victim row's PRAC counter
    b->PRAC[victim_row] = 0;

    // Increment PRAC values of neighboring rows
    for (int offset = -2; offset <= 2; offset++) {
        if (offset == 0) continue;
        
        int neighbor = victim_row + offset;
        if (neighbor >= 0 && neighbor < (int) b->num_rows) {
            b->PRAC[neighbor]++;
        }
    }

    // Clear the queue entry
    b->moat_queue[0] = -1;
    
    // Increment mitigation counter
    b->s_mitigs++;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////
