#!/bin/bash
#SBATCH  --partition=p_hpca4se
#SBATCH -x huberman 
#SBATCH -J SPIF_WF
#SBATCH -N 1-1 
#SBATCH --exclusive
#SBATCH -c 26
#SBATCH --threads-per-core=1 
#SBATCH --time=010:00:00 
#SBATCH --error=job.%J.WF.err 
#SBATCH --output=job.%J.WF.out
#SBATCH --mem-per-cpu=1500
#SBATCH --cpu_bind=verbose

cat $PE_HOSTFILE
echo "--------------------------"
# Module load section
#export I_MPI_PROCESS_MANAGER=mpd
export I_MPI_PMI_LIBRARY=/opt/openmpi-1.6.4/lib/libpmi.so
module load openmpi/1.6.4 boost/1.49.0 windninja/2.1.1  

echo "--------------------------"
## Compilacio carregant NOMES openmpi, no gcc.
make clean
make all
#echo mpirun -np $NSLOTS /home/cbrun/SPIF/genetic /home/cbrun/SPIF/Jonquera/pob0/datos_WF.ini 
srun -n 26 mpirun -np 26 /home/cbrun/SPIF/genetic /home/cbrun/SPIF/Jonquera/pob0/datos_WF.ini 
##echo srun -n 26 /home/cbrun/SPIF/genetic /home/cbrun/SPIF/Jonquera/pob0/datos_WF.ini 
##srun -n 26 /home/cbrun/SPIF/genetic /home/cbrun/SPIF/Jonquera/pob0/datos_WF.ini 

exit 0
