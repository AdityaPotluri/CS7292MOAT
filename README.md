## Lab-1 Rowhammer Mitigations
     
In this lab assignment, you will implement a rowhammer mitigation MOAT (ASPLOS'25) that uses Per-Row Activation Counters (PRAC).

### Task-A - Implement PRAC without Mitigations and compare performance with No-PRAC  [2 points]
The first task is to implement PRAC. Specifically, you have to perform the following 3 tasks in drambank.c
1. Allocate and Initialize the PRAC Counters
2. Update the PRAC Counters on ACT
3. Reset the PRAC Counters on REF

#### **Grading Criteria:**  
We will use the value of the statistics DRAM_LOG2_ACTS_* to award points. +2 points if the values match the correct answer (+-5% error is acceptable), otherwise 0.

#### **Deliverables:**
You need to report the AVG_IPC with and without PRAC (use the `-noprac` flag to run without the timing constraints of PRAC). Additionally, you need to report all the DRAM_LOG2_ACTS_* stats. 

### Task-B - Implement MOAT [5 points]
The second task is to implement MOAT that uses PRAC to protect against RH. Spefically you have to perform the following tasks:
1. Initialize the `moat_queue`. What should be the size of this queue?
2. Implement `dram_moat_check_insert`, the function template in drambank.c has more details
3. Implement `dram_moat_mitig`, the function template in drambank.c has more details

#### **Grading Criteria:**
Evaluate MOAT with ATH=64 and ATH=128. Assume Blast Radius BR=2.
We will use the value of the statistics DRAM_MITIGS, DRAM_ALERTS and DRAM_MITIGPTREFW to award points. +5 points if the values match the correct answer (+-5% error is acceptable), otherwise 0.

#### **Deliverables:**
You need to report DRAM_MITIGS, DRAM_ALERTS and DRAM_MITIGPTREFW for ATH=64 and 128.

### Task-C - Implement MOAT-RP [3 points]
See Section-9 of the paper. You will focus on the number of times the mitigation is invoked and PRAC counters. Assume tEPOCH as 180ns. 

#### **Grading Criteria:**  
The points for this task will be awarded based on your results for the DRAM_MITIGS, DRAM_ALERTS and DRAM_MITIGPTREFW. 

#### **Deliverables:**
You need to report DRAM_MITIGS, DRAM_ALERTS and DRAM_MITIGPTREFW for ATH=64 and 128.

### Files for Submission.
You will submit one tar with three folders inside the tar: `src_lab1_a`, `src_lab1_b`, `src_lab1_c`, each containing the entire source code (`src` folder) for the particular task. Please make sure that your code compiles and runs on either the Ubuntu or Red-Hat machines provided by [ECE](https://help.ece.gatech.edu/labs/names)  or [CoC](https://support.cc.gatech.edu/facilities/general-access-servers). We will compile, run and evaluate your code on similar machines.  You should also submit a 2 page report (submitted as PDF) capturing the high-level details of your implementation and tables capturing the key results for parts (A)(B)(C) averaged across all the workloads. 
  
### Collaboration / Plagiarism Policy
- You are allowed to discuss the problem and potential solution directions with your peers.
- You are not allowed to discuss or share any code with your peers, or copy code from any source on the internet. All work you submit MUST be your own.
- You are also not allowed to post the problems or your solutions on the online.
- All submitted code will be checked for plagiarism against other submissions (and other sources from the internet) and all violations will receive 0 on the assignment.


## DRAM-RH-SIM v1.0
A fast DRAM simulator for evaluating Rowhammer defenses.

### Instructions to use the Simulator
- **Download Traces**: Download traces from the Files in canvas under the `traces-lab1` folder.
- **Compile**: To compile, use `cd src/; make all`. This will compile the source code to create `sim` executable.
- **Run**; To run the simulator with a workload's execution trace (e.g. `../TRACES/bwaves_06.mat.gz`) running on 8 Cores of a CPU, use ``./sim -mtapp 8 -inst_limit 100000000 ../TRACES/bwaves_17.mat.gz > ../RESULTS/BASELINE.8C.8MB/bwaves_17.res``. 
  - You can run the simulator with any of the traces in the `../TRACES/` folder with the same trace or different traces running on the 8 CPU cores. 
  - `../scripts/runall.sh` provides the command for running one benchmark and also all benchmarks in parallel.
- **Get Statistics**: To get statistics, you can check the output of the simulator. For the bwaves_17 run above, you can see `../RESULTS/BASELINE.8C.8MB/bwaves_17.res` for statistics like `SYS_CYCLES : 2155238427`. 
  - To report a statistic (SYS_CYCLES) for all benchmarks and several results folders, use `cd ../scripts;  perl getdata.pl -w lab1 -amean -s SYS_CYCLES  -d ../RESULTS/BASELINE.8C.8MB <ANOTHER RESULTS FOLDER>`. 
  - `getdata.pl` has several other useful flags including `-gmean` for geometric mean, `-n` for normalizing results of one folder with another, `-mstat` to multiply all stats with a constant value and more.

