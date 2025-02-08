RM        := /bin/rm -rf
SIM       := ./sim
CC        := g++
# CFLAGS    := -lz -O2 -lm  -W -Wall -Wno-deprecated -Wno-unknown-warning-option -Wno-self-assign -Wno-unused-parameter -Wno-unused-but-set-variable
CFLAGS    := -g -O3 -lz -lm  -W -Wall -Wno-deprecated -Wno-unknown-warning-option -Wno-self-assign -Wno-unused-parameter -Wno-unused-but-set-variable
DFLAGS    :=  -fsanitize=address
PFLAGS    := -pg




all:  
	${CC} ${CFLAGS} dram.c dramchannel.c drambank.c mcore.c os.c  mcache.c sim.c memsys.c -o ${SIM} -lz

dbg:  
	${CC} ${CFLAGS} ${DFLAGS} dram.c dramchannel.c drambank.c  mcore.c os.c  mcache.c sim.c memsys.c -o ${SIM} -lz

clean: 
	$(RM) ${SIM} *.o
