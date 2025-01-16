
#####################################################################################################

fw=64 # FireWidth, set this to 7 if running on 8-core laptop (sims may take 8-10 hours to finish)
suite="moat21"
simopt="-dramrefpolicy 2 -inst_limit 1000000000 -mtapp 8 -drammappolicy 1 -memclosepage" 

######################################################################################################

echo "Compiling simulator ... this may take a few seconds"

cd ../src ; make; cd ../SCRIPTS

sleep 10

echo "Running sims ... this may take about 2-3 hours"

./runall.pl --w $suite --f $fw  --d "../RESULTS/PRAC" --o  " $simopt ";

./runall.pl --w $suite --f $fw  --d "../RESULTS/MOAT.ATH064" --o  "$simopt -enablemoat -refspermitig 5 -moat_eth 32 -moat_ath 65 "; 

./runall.pl --w $suite --f $fw  --d "../RESULTS/MOAT.ATH128" --o  "$simopt -enablemoat -refspermitig 5 -moat_eth 64 -moat_ath 129 ";

echo "Sleeping for 15 minutes to ensure ALL sims finish"

sleep 900

echo "Perf sims most likely done ... Please run top and check if any sim.bin is still running"


######################################################################################################
# Run the below command ONLY after the performance simulations have completed (confirm with "top")
######################################################################################################

./graphs.sh
