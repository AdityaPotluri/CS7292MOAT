#ifndef __PARAMS_H__
#define __PARAMS_H__

#include "global_types.h"
#include <string.h>

#define PRINT_DOTS   1
#define DOT_INTERVAL 1000000

uns64  CORE_WIDTH   =    4;
uns64  ROB_SIZE     =    256;


uns64       USE_IMAT_TRACES = 1; // second field is 4byte iaddr
uns64       TRACE_LIMIT     = (2*1000*1000*1000); // Max 2B memory access
uns64       INST_LIMIT      = 100*1000*1000; // set default 
uns64       NUM_THREADS     = 0;
uns64       MT_APP_THREADS  = 0;
uns64       LINESIZE        = 64; 
uns64       OS_PAGESIZE     = 4096; 


uns64       L3_SIZE_KB      = 8192; 
uns64       L3_ASSOC        = 16; 
uns64       L3_LATENCY      = 24; // cycles
uns64       L3_REPL         = 0; //0:LRU 1:RND 2:SRRIP
uns64       L3_PERFECT      = 0; //simulate 100% hit rate for L3


//--- DDR5 5200MTPS (2.6GHZ)
uns64       MEM_SIZE_MB     = 32768; 
uns64       MEM_CHANNELS    = 2;
uns64       MEM_BANKS       = 64; // Total banks in memory, not  per channel (16x2 ranks here)
uns64       MEM_PAGESIZE    = 8192; //Size of a DRAM Row
uns64       MEM_CLOSEPAGE   = 0;

uns64       DRAM_REF_POLICY= 1; /* SAME BANK */
uns64       DRAM_MAP_POLICY=1; // SKYLAKE
uns64       DRAM_SCHED_POLICY=0; // 0:FCFS 1:FR-FCFS
uns64       DRAM_MOP_GANGSIZE = 8; 

// all DRAM latencies specified as ns x 4 (4GHz processor)
uns64       tRC=52*4;  // No-PRAC 48*4;
uns64       tRFC=410*4;
uns64       tACT=12*4;
uns64       tCAS=12*4;
uns64       tPRE=36*4; // No-PRAC 16*4;
uns64       tRAS=16*4; // No-PRAC 32*4;
uns64       tRDRD=3*4;
uns64       tREFI=3900*4;
uns64       tREFW=32*1000*1000*4;
uns64       tBUS=3*4;
uns64       tALERT=350*4; //-- only 350ns of stall out of 530


uns64       DRAM_MAX_TOPEN   = 3900*4;
uns64       ENABLE_MOAT      =0; // MOAT
uns64       ENABLE_RP        =1; // OAT-RP	
uns64       MOAT_LEVEL       =1; // MOAT ABO LEVEL
uns64       REFS_PER_MITIG   =5; // mitigation rate
uns64       RESET_CTR_ON_REF = 0; // MOAT
uns64       MOAT_ATH         = 64;
uns64       MOAT_ETH         = 32;


//-- Rowhammer Related --
uns64       MEM_RSRV_MB      = 0; //last 1 GB is reserved (for CRAM counters in DRAM)
uns64       RH_THRESHOLD_ACT    = 1024; //number of activations beyond which rowhammer bitflips might be possible.
//-----------------------



uns64       OS_NUM_RND_TRIES=5; // page mapping (try X random invalids first)
uns64       RAND_SEED       = 1234;

uns64       cycle;
uns64       last_printdot_cycle;
char        addr_trace_filename[256][1024];
int         num_threads = 0;


/***************************************************************************************
 * Functions
 ***************************************************************************************/


void die_usage() {
    printf("Usage : sim [options] <FAT_trace_0> ... <FAT_trace_n> \n\n");
    printf("Trace driven DRAM-cache based memory-system simulator\n");

    printf("   Options\n");
    printf("               -imat                 Use IMAT traces (Default:0, but confirm)\n");
    printf("               -inst_limit  <num>    Set instruction limit (Default: 0)\n");

    printf("               -l3sizekb    <num>    Set L3  Cache size to <num> KB (Default: 1MB)\n");
    printf("               -l3sizemb    <num>    Set L3  Cache size to <num> MB (Default: 1MB)\n");
    printf("               -l3assoc     <num>    Set L3  Cache assoc <num> (Default: 16)\n");
    printf("               -l3latency   <num>    Set L3  Caches latency in cycles (Default: 15)\n");
    printf("               -l3repl      <num>    Set L3  Caches replacement policy [0:LRU 1:RND 2:SRRIP] (Default:2)\n");
    printf("               -l3perfect            Set L3  to 100 percent hit rate(Default:off)\n");
    printf("               -memclosepage         Set DRAM to close page (Default:off)\n");
    printf("               -drammaxtopen         Set DRAM Max Row Open Time (Default:unlimited)\n");
    printf("               -os_numrndtries <num>    OS randomly tries X times to find invalid page (Default: 5)\n");

    printf("               -os_pagesize <num>    Size of OS page  (Default: 4096)\n");

    printf("               -rand_seed <num>         Seed for Random PRNG (Default: 1234)\n");
    
    printf("               -mtapp         <num>    Number of threads in rate mode (Default: 1)\n");

	printf("               -drammop         <num>     Size of DRAM Minimalist Open Page <LinesInRowBuf> (Default: off)\n");
 	printf("               -enablemoat                 Set MOAT mitigation (Default:off)\n");
 	printf("               -refspermitig      <num>    Refs per mitig for in-DRAM Mitigation  (Default: 2)\n");
  	printf("               -resetctronref             Reset ctr on ref for MOAT (Default:off)\n");
  	printf("               -moat_ath        <num>    MOAT ATH  (Default: 100-12)\n");
  	printf("               -moat_eth        <num>    MOAT ETH  (Default: 50)\n");
	printf("               -noprac					 No PRAC (Default:off)\n");
    exit(0);
}


/***************************************************************************************
 * Functions
 ***************************************************************************************/

												          
void die_message(const char * msg) {
    printf("Error! %s. Exiting...\n", msg);
    exit(1);
}




/***************************************************************************************
 * Functions
 ***************************************************************************************/



void read_params(int argc, char **argv){
  int ii;

    //--------------------------------------------------------------------
    // -- Get command line options
    //--------------------------------------------------------------------    
    for ( ii = 1; ii < argc; ii++){
	if (argv[ii][0] == '-') {	    
	  if (!strcmp(argv[ii], "-h") || !strcmp(argv[ii], "-help")) {
		die_usage();
	    }

	   else if (!strcmp(argv[ii], "-inst_limit")) {
	     if (ii < argc - 1) {		  
	       INST_LIMIT = atoi(argv[ii+1]);
	       ii += 1;
	     }
	   }

	   else if (!strcmp(argv[ii], "-robsize")) {
	     if (ii < argc - 1) {		  
	       ROB_SIZE = atoi(argv[ii+1]);
	       ii += 1;
	     }
	   }
	  
	   else if (!strcmp(argv[ii], "-os_pagesize")) {
		if (ii < argc - 1) {		  
		    OS_PAGESIZE = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }
	  
	    else if (!strcmp(argv[ii], "-l3perfect")) {
	      L3_PERFECT=1; 
	    }

		else if (!strcmp(argv[ii], "-noprac")) {
	      tRC=48*4;  // No-PRAC 48*4;
		  tPRE=16*4; // No-PRAC 16*4;
		  tRAS=32*4; // No-PRAC 32*4;
	    }


	    else if (!strcmp(argv[ii], "-l3repl")) {
		if (ii < argc - 1) {		  
		    L3_REPL = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-l3sizekb")) {
		if (ii < argc - 1) {		  
		    L3_SIZE_KB = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-l3sizemb")) {
		if (ii < argc - 1) {		  
		    L3_SIZE_KB = 1024*atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-l3assoc")) {
		if (ii < argc - 1) {		  
		    L3_ASSOC = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }


	    else if (!strcmp(argv[ii], "-l3latency")) {
		if (ii < argc - 1) {		  
		    L3_LATENCY = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-imat")) {
	      USE_IMAT_TRACES = 1;
	    }

	  
	    else if (!strcmp(argv[ii], "-memclosepage")) {
	      MEM_CLOSEPAGE = 1;
	    }

	    else if (!strcmp(argv[ii], "-drammaxtopen")) {
		if (ii < argc - 1) {		  
		    DRAM_MAX_TOPEN = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	   else if (!strcmp(argv[ii], "-rand_seed")) {
	      if (ii < argc - 1) {		  
		RAND_SEED = atoi(argv[ii+1]);
		ii += 1;
	      }
	    }

	    else if (!strcmp(argv[ii], "-mtapp")) {
		if (ii < argc - 1) {		  
		    MT_APP_THREADS = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-dramrefpolicy")) {
		if (ii < argc - 1) {		  
		    DRAM_REF_POLICY = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-drammappolicy")) {
	      if (ii < argc - 1) {		  
		DRAM_MAP_POLICY = atoi(argv[ii+1]);
		ii += 1;
	      }
	    }

	    else if (!strcmp(argv[ii], "-dramschedpolicy")) {
	      if (ii < argc - 1) {		  
		DRAM_SCHED_POLICY = atoi(argv[ii+1]);
		ii += 1;
	      }
	    }

	    else if (!strcmp(argv[ii], "-drammop")) {
	      if (ii < argc - 1) {		  
		DRAM_MOP_GANGSIZE = atoi(argv[ii+1]);
		ii += 1;
	      }
	    }

	  
	    else if (!strcmp(argv[ii], "-moatlevel")) {
	      if (ii < argc - 1) {		  
		MOAT_LEVEL = atoi(argv[ii+1]);
		ii += 1;
	      }
	      tALERT=350*4*MOAT_LEVEL; // rescale tALERT
	    }

	  
	    else if (!strcmp(argv[ii], "-rh_thresh")) {
		if (ii < argc - 1) {		  
		    RH_THRESHOLD_ACT = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-limit")) {
		if (ii < argc - 1) {		  
		    TRACE_LIMIT = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }


	    else if (!strcmp(argv[ii], "-tRC")) {
		if (ii < argc - 1) {		  
		    tRC = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-tRFC")) {
	        if (ii < argc - 1) {		  
		    tRFC = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-tACT")) {
	        if (ii < argc - 1) {		  
		    tACT = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-tCAS")) {
	        if (ii < argc - 1) {		  
		    tCAS = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-tPRE")) {
	        if (ii < argc - 1) {		  
		    tPRE = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-tRAS")) {
	        if (ii < argc - 1) {		  
		    tRAS = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-tRDRD")) {
	        if (ii < argc - 1) {		  
		    tRDRD = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-tREFI")) {
	      if (ii < argc - 1) {		  
		tREFI = atoi(argv[ii+1]);
		ii += 1;
	      }
	    }
	  
	    else if (!strcmp(argv[ii], "-tREFW")) {
	        if (ii < argc - 1) {		  
		    tREFW = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }
	  
	    else if (!strcmp(argv[ii], "-resetctronref")) {
	      RESET_CTR_ON_REF = 1;
	    }

	    else if (!strcmp(argv[ii], "-enablemoat")) {
	      ENABLE_MOAT = 1;
	    }

		else if (!strcmp(argv[ii], "-enablerp")) {
			ENABLE_RP = 1;
		}
	  
	    else if (!strcmp(argv[ii], "-refspermitig")) {
		if (ii < argc - 1) {		  
		    REFS_PER_MITIG = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-moat_ath")) {
		if (ii < argc - 1) {		  
		    MOAT_ATH = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else if (!strcmp(argv[ii], "-moat_eth")) {
		if (ii < argc - 1) {		  
		    MOAT_ETH = atoi(argv[ii+1]);
		    ii += 1;
		}
	    }

	    else {
		char msg[256];
		sprintf(msg, "Invalid option %s", argv[ii]);
		die_message(msg);
	    }
	}
	else if (num_threads<MAX_THREADS) {
	    strcpy(addr_trace_filename[num_threads], argv[ii]);
	    num_threads++;
	    NUM_THREADS = num_threads;
	}
	else {
	    char msg[256];
	    sprintf(msg, "Invalid option %s", argv[ii]);
	    die_message(msg);
	}    
    }
	    
    //--------------------------------------------------------------------
    // Error checking
    //--------------------------------------------------------------------
    if (num_threads==0) {
	die_message("Must provide valid at least one addr_trace");
    }

    if( MT_APP_THREADS ){
	num_threads = MT_APP_THREADS;
	NUM_THREADS = MT_APP_THREADS;
	for(ii=1; ii<num_threads; ii++){
	    strcpy(addr_trace_filename[ii], addr_trace_filename[0]);
	}
    }
}





#endif  
