#!/bin/bash
#$ -S /bin/bash
#$ -N FS_PRED
#$ -A FS_PRED
#$ -pe mpi 1
#$ -cwd
#$ -notify
#$ -m e
#$ -q cluster.q@@newnodes 
#$ -l slots_node=4
####$ -l excl
####$ -q cluster.q
#$ -R y

#export OMP_NUM_THREADS=1

cat $PE_HOSTFILE
echo "--------------------------"
# Module load section
module av 
module load gcc/4.7.2
module load papi/5.1.0 
module load gdal/1.9.2 
module load netcdf/4.2.1.1 
module load proj/4.8.0 
module load boost/1.53.0 
module load openmpi/1.6.4
module list

echo "--------------------------"

/home/sgeadmin/two_stage_framework/farsite/farsite4P -i /home/cbrun/Framework/FFSS/Jonquera/pob$1/output/settings_10_$2.txt -f 4 -t 1 -g 2 -n 3 -w 1 -p 120m

exit 0
