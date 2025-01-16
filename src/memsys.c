#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "externs.h"
#include "memsys.h"
#include "mcore.h"

extern MCore *mcore;


////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

MemSys *memsys_new(void){
    MemSys *m = (MemSys *) calloc (1, sizeof (MemSys));

    // init main memory DRAM
    m->mainmem = dram_new(); 
    sprintf(m->mainmem->name, "MEM");
    m->lines_in_mainmem_rbuf = MEM_PAGESIZE/LINESIZE; // static
   
    return m;
}


//////////////////////////////////////////////////////////////////////////
// NOTE: ACCESSES TO THE MEMORY USE LINEADDR OF THE CACHELINE ACCESSED
//////////////////////////////////////////////////////////////////////////

Flag memsys_access(MemSys *m, Addr lineaddr,  uns coreid, uns robid, uns64 inst_num, Flag wb){
  Flag retval=FALSE;
  retval = dram_insert(m->mainmem, lineaddr, coreid, robid, inst_num, wb);
  return retval;
}


//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void  memsys_cycle(MemSys *m){
  dram_cycle(m->mainmem); 
}

//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void  memsys_print_state(MemSys *m){
  dram_print_state(m->mainmem);
}

//////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////

void  memsys_print_stats(MemSys *m){
  dram_print_stats(m->mainmem);
}

