#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "mcore.h"
#include "externs.h"
#include "dram.h"



uns   DRAM_ID=0;

extern uns64 cycle;
extern MCore *mcore;

extern uns64 DRAM_MAX_TOPEN;





////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

DRAM*   dram_new(void){
  uns ii;
  
  DRAM *d = (DRAM *) calloc (1, sizeof (DRAM));
  d->num_banks       = MEM_BANKS;
  d->memsize         = MEM_SIZE_MB*1024*1024;
  d->rowbuf_size     = MEM_PAGESIZE;
  d->num_channels    = MEM_CHANNELS;
  d->num_rowbufs     = d->memsize/d->rowbuf_size;
  d->lines_in_rowbuf = d->rowbuf_size/LINESIZE; // DRAM access granularity is linesize
  d->rowbufs_in_bank = d->num_rowbufs/d->num_banks;
  d->lines_in_mem    = d->memsize/LINESIZE;
  d->rowbufs_in_mem  = d->memsize/d->rowbuf_size;
  d->banks_in_channel = MEM_BANKS/MEM_CHANNELS;
  d->refmode          = (DRAM_RefType) DRAM_REF_POLICY;

  for(ii=0; ii< d->num_channels ; ii++){
    d->channel[ii] = dram_channel_new(ii, d->banks_in_channel, d->rowbufs_in_mem/MEM_CHANNELS);
  }

  printf("Size: %llu GB Channels: %llu and BanksPerChannel: %llu\n", d->memsize/(1<<30), d->num_channels, d->num_banks/d->num_channels );

  d->id=DRAM_ID++;
  sprintf(d->name, "DRAM_%02d",d->id);

  
  return d;
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

Flag   dram_insert(DRAM *d, Addr lineaddr, uns coreid, uns robid, uns64 inst_num, Flag wb){
  uns64 mybankid, myrowbufid, mychannelid;
  Flag retval = TRUE;
  DRAM_ReqType reqtype = DRAM_REQ_RD;

  if(wb){
    reqtype = DRAM_REQ_WB;
  }
  
  
  assert(lineaddr < d->lines_in_mem); //we get physical DRAM lineid

  // parse address bits and get my bank
  dram_parseaddr(d,lineaddr,&myrowbufid,&mybankid,&mychannelid);
  
  retval = dram_channel_insert(d->channel[mychannelid], mybankid, reqtype, myrowbufid, coreid, robid, inst_num);
  return retval; 
}



////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////

void    dram_parseaddr(DRAM *d, Addr lineaddr, uns64 *myrowbufid, uns64 *mybankid, uns64 *mychannelid){
  DRAM_MappingPolicy policy=(DRAM_MappingPolicy)DRAM_MAP_POLICY;
  
  if(policy == DRAM_MAP_COFFEELAKE){
    uns64 global_row_id = lineaddr/d->lines_in_rowbuf;
    uns64 my_channel_id = global_row_id % d->num_channels;
    uns64 shifted_addr1 = global_row_id / d->num_channels;
    uns64 my_local_bankid = shifted_addr1 % d->banks_in_channel;// banks are striped in channels
    uns64 shifted_addr2 = shifted_addr1 / d->banks_in_channel;
    
    *myrowbufid  = shifted_addr2;
    *mybankid    = my_local_bankid;
    *mychannelid = my_channel_id;
    return;
  }

  
   if(policy == DRAM_MAP_SKYLAKE){
     uns64 linepair_id = lineaddr/2;
     uns64 my_channel_id = linepair_id % d->num_channels;
     uns64 shifted_addr1 = linepair_id / d->num_channels;
     uns64 shifted_addr2 = shifted_addr1 / (d->lines_in_rowbuf/2); // lines in row
     uns64 my_local_bankid = shifted_addr2 % d->banks_in_channel;// banks are striped in channels
     uns64 shifted_addr3 = shifted_addr2 / d->banks_in_channel;

     *myrowbufid  = shifted_addr3;
     *mybankid    = my_local_bankid;
     *mychannelid = my_channel_id;
     return;
   }

   
  if(policy == DRAM_MAP_MOP){
    uns64 gang_id = (lineaddr/DRAM_MOP_GANGSIZE);
    uns64 my_channel_id = gang_id % d->num_channels;
    uns64 shifted_addr1 = gang_id / d->num_channels;
    uns64 my_local_bankid = shifted_addr1 % d->banks_in_channel;// banks are striped in channels
    uns64 shifted_addr2 = shifted_addr1 / d->banks_in_channel;

    *myrowbufid  = shifted_addr2/(d->lines_in_rowbuf/DRAM_MOP_GANGSIZE);
    *mybankid    = my_local_bankid;
    *mychannelid = my_channel_id;
    return;
  }

  if(policy == DRAM_MAP_ZEN){
    uns64 global_addr = lineaddr;
    uns64 my_channel_id = global_addr % d->num_channels;
    uns64 shifted_addr1 = global_addr / d->num_channels;
    uns64 shifted_addr2 = shifted_addr1/2; // skip 1-bit
    uns64 my_local_bankid = shifted_addr2 % d->banks_in_channel;// banks are striped in channels
    uns64 shifted_addr3 = shifted_addr2 / d->banks_in_channel;
    
    *myrowbufid  = shifted_addr3/(d->lines_in_rowbuf/2);
    *mybankid    = my_local_bankid;
    *mychannelid = my_channel_id;
    return;
  }

  assert(0); // other policy not implemented yet

}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


void dram_cycle(DRAM *d){
  uns ii,jj;
  for(ii=0; ii< d->num_channels; ii++){
    dram_channel_cycle(d->channel[ii]);
  }

  uns64 trefw_delta = cycle - d->last_trefw_cycle;
  
  if( trefw_delta > tREFW){
    dram_gather_stats(d);
    d->last_trefw_cycle = cycle;
    d->num_trefw++;
  }

  //---- handling refresh every TREFI -------------------
  uns64 trefi_delta = cycle - d->last_trefi_cycle;
  
  if( trefi_delta > tREFI){
    d->last_trefi_cycle = cycle;
    d->num_trefi++;
    
    if(d->refmode == DRAM_REF_SB){
      for(ii=0; ii< d->num_channels; ii++){
	for(jj=0; jj < d->num_banks/d->num_channels; jj++){
	  dram_bank_insert(d->channel[ii]->bank[jj],  DRAM_REQ_REF, 0, 0, 0,0);
	}
      }
    }

    if(d->refmode == DRAM_REF_AB){
      for(ii=0; ii< d->num_channels; ii++){
	uns64 max_bank_ready_cycle = cycle;
	for(jj=0; jj < d->num_banks/d->num_channels; jj++){
	  if(d->channel[ii]->bank[jj]->sleep_cycle > max_bank_ready_cycle){
	    max_bank_ready_cycle = d->channel[ii]->bank[jj]->sleep_cycle;
	  }
	}
	for(jj=0; jj < d->num_banks/d->num_channels; jj++){
	  dram_bank_refresh(d->channel[ii]->bank[jj],max_bank_ready_cycle); // no bank-q
	}
      }
    }
    
  }

}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


void dram_gather_stats(DRAM *d){
  uns ii,jj,kk,zz;
  
  for(ii=0; ii< d->num_channels; ii++){
    for(jj=0; jj < d->num_banks/d->num_channels; jj++){
      for(kk=0; kk < d->num_rowbufs/d->num_banks; kk++){
	      uns val = d->channel[ii]->bank[jj]->PRAC[kk];
	      if(val> 1){
          uns log2val = (int)(log2(val));
          if(log2val>13){
            log2val=13;
      }
	  for(zz=1; zz<=log2val; zz++){
	    //d->s_ACT_dist[zz]++; // unique-rows-count
	    d->s_ACT_dist[zz]+= (int)(val/(1<<zz)); // dynamic count
	  }
	}
	d->channel[ii]->bank[jj]->PRAC[kk]=0; // reset at tREFW
      }
    }
  }

}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


void dram_print_state(DRAM *d){
  for(uns ii=0; ii< d->num_channels; ii++){
    dram_channel_print_state(d->channel[ii]);
  }
  printf("\n");
}

////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////


void dram_print_stats(DRAM *d)
{
 uns ii,jj;
  
  for(ii=0; ii< d->num_channels; ii++){
    d->s_tot_ALERT += d->channel[ii]->s_ALERT;
    d->s_tot_bus_delay += d->channel[ii]->s_bus_time;
    
    for(jj=0; jj < d->num_banks/d->num_channels; jj++){
      d->s_tot_ACT +=  d->channel[ii]->bank[ii]->s_ACT;
      d->s_tot_REQ +=  d->channel[ii]->bank[ii]->s_REQ;
      d->s_tot_service_delay +=  d->channel[ii]->bank[ii]->s_service_delay;
      d->s_tot_wait_delay +=  d->channel[ii]->bank[ii]->s_wait_delay;
      d->s_tot_mitigs  +=  d->channel[ii]->bank[ii]->s_mitigs;
    }
  }

  double actpreq = 100*(double)( d->s_tot_ACT)/(double)(d->s_tot_REQ);
  double avg_service_delay =  (double)d->s_tot_service_delay/(double)(d->s_tot_REQ);
  double avg_wait_delay =  (double)d->s_tot_wait_delay/(double)(d->s_tot_REQ);
  double avg_bus_delay =  (double)d->s_tot_bus_delay/(double)(d->s_tot_REQ);
  double trefi_per_alert = (double)d->num_trefi/(double)(d->s_tot_ALERT);

  double mitigs_per_trefw = 8192*(double)(d->s_tot_mitigs)/(double)(d->num_trefi*d->num_banks);
   
  char header[256];
  sprintf(header, "DRAM");
  
  printf("\n%s_ACT         \t : %llu",  header,  d->s_tot_ACT);
  printf("\n%s_REQ         \t : %llu",  header,  d->s_tot_REQ);
  printf("\n%s_RBHR        \t : %4.2f",  header, 100.0-actpreq);
  printf("\n%s_AVG_SDELAY  \t : %4.2f",  header, avg_service_delay);
  printf("\n%s_AVG_WDELAY  \t : %4.2f",  header, avg_wait_delay);
  printf("\n%s_AVG_TDELAY  \t : %4.2f",  header, avg_service_delay+avg_wait_delay);
  printf("\n%s_AVG_BUSDELAY\t : %4.2f",  header, avg_bus_delay);
  printf("\n%s_NUM_TREFI   \t : %llu",  header,  d->num_trefi);
  printf("\n%s_NUM_TREFW   \t : %llu",  header,  d->num_trefw);
  printf("\n%s_TREFIPALERT \t : %4.2f",  header, trefi_per_alert);

  for(ii=5; ii<=13; ii++){
    printf("\n%s_LOG2_ACTS_%02u\t : %llu",  header, ii, d->s_ACT_dist[ii]/(d->num_trefw*d->num_banks));
  }

  printf("\n");

  printf("\n%s_ALERT       \t : %llu",  header,  d->s_tot_ALERT);
  printf("\n%s_MITIGS      \t : %llu",  header,  d->s_tot_mitigs);
  printf("\n%s_MITIGPTREFW \t : %6.2f", header,  mitigs_per_trefw);


  printf("\n");
}
