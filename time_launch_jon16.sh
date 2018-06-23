#!/bin/bash -x
SBATCH -x robin,huberman,penguin,sandman

MPIARGS="-n 9 --report-bindings --display-allocation --report-bindings"
ulimit -s unlimited
cat $PE_HOSTFILE
echo "--------------------------"

source  /opt/Modules/3.2.9/init/Modules4bash.sh
module rm openmpi-x86_64
module load openmpi/1.6.5
module load windninja/2.1.1
module load geos/3.3.8 proj/4.8.0 netcdf/4.3.0 gdal/1.9.2
module load windninja/2.1.1
module list
export CLASSPATH="/home/tartes/Soft/Weka/weka-3-6-10/weka.jar:/scratch/078-hpca4se-comp/tomas/SPIF/Andres/:$CLASSPATH"
echo "--------------------------"
mkdir $SLURM_JOB_ID
cd $SLURM_JOB_ID
mkdir Traces
mkdir Shapes
mkdir TOA
cd ..

cd /home/sgeadmin/two_stage_framework/farsite
make clean
make normal
cd /home/sgeadmin/two_stage_framework/proba5/SPIF
make clean
make normal

echo "mpirun ${MPIARGS} /home/sgeadmin/two_stage_framework/proba5/SPIF $SLURM_JOB_ID $1"
mpirun ${MPIARGS} /home/sgeadmin/two_stage_framework/proba5/SPIF/genetic $SLURM_JOB_ID $1

InError=`grep "GenAlErroPath" $1 | cut -d'=' -f2 | tr -d " \t\n\r"`

gnuplot -e "filename='$2/Error_$SLURM_JOB_ID.ps'" -e "inputFile='${InError}GenAlError_$SLURM_JOB_ID.dat'" -e "JobID='JOB_$SLURM_JOB_ID'" Utils/gnuExample.gnu

cp $1 $2/
cp SPIF_$SLURM_JOB_ID.* $2
cat $2/Trace/*.dat | sort -n -k4 > $2/Trace.txt
cp $InError/GenAlError_$SLURM_JOB_ID.dat $2

tar -czf Summary_$SLURM_JOB_ID.tar.gz $2 

killall -u sgeadmin
exit 0
