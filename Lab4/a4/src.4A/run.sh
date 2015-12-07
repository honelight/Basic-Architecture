
######################################################################################
# This scripts runs all three traces
# You will need to uncomment the configurations that you want to run
# the results are stored in the ../results/ folder 
######################################################################################


########## ---------------  A.1 ---------------- ################

./sim -mode 1 ../traces/bzip2.mtr.gz > ../results/A1.bzip2.res 
./sim -mode 1 ../traces/mcf.mtr.gz   > ../results/A1.mcf.res 
./sim -mode 1 ../traces/lbm.mtr.gz  > ../results/A1.lbm.res 


########## ---------------  A.2 ---------------- ################

./sim -mode 1 -linesize 32 ../traces/bzip2.mtr.gz > ../results/A2.L32.bzip2.res 
./sim -mode 1 -linesize 32 ../traces/mcf.mtr.gz   > ../results/A2.L32.mcf.res 
./sim -mode 1 -linesize 32 ../traces/lbm.mtr.gz  > ../results/A2.L32.lbm.res 

./sim -mode 1 -linesize 128 ../traces/bzip2.mtr.gz > ../results/A2.L128.bzip2.res 
./sim -mode 1 -linesize 128 ../traces/mcf.mtr.gz   > ../results/A2.L128.mcf.res 
./sim -mode 1 -linesize 128 ../traces/lbm.mtr.gz  > ../results/A2.L128.lbm.res 


########## ---------------  A.3 ---------------- ################

./sim -mode 1 -repl 1 ../traces/bzip2.mtr.gz > ../results/A3.R1.bzip2.res 
./sim -mode 1 -repl 1 ../traces/mcf.mtr.gz   > ../results/A3.R1.mcf.res 
./sim -mode 1 -repl 1 ../traces/lbm.mtr.gz  > ../results/A3.R1.lbm.res 


echo "All Done. Check the .res file in ../results directory";

