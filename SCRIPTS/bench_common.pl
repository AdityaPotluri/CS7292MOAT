#!/usr/bin/perl -w

#******************************************************************************
# Benchmark Sets
# ************************************************************

%SUITES = ();


$SUITES{'gap6'}      = 
	      'cc
	       pr
	       bfs
	       tc
               bc
               sssp';


# ACT of at least 0.5 per 1K inst
$SUITES{'spec2k17act'}      = 
    'bwaves_17
  fotonik3d_17
        lbm_17
        mcf_17
    omnetpp_17
       roms_17
     parest_17
         xz_17
  cactuBSSN_17
       cam4_17
    blender_17
  xalancbmk_17
        wrf_17
       x264_17
        gcc_17';



$SUITES{'moat21'}      = 
    "$SUITES{'spec2k17act'}"."\n".
              "$SUITES{'gap6'}";
