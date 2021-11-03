#!/bin/bash


#SBATCH --job-name=JobExample1       #Set the job name to "JobExample1"
#SBATCH --time=01:00:00              #Set the wall clock limit to 1hr and 30min
#SBATCH --ntasks=1                   #Request 1 task
#SBATCH --mem=2560M                  #Request 2560MB (2.5GB) per node
#SBATCH --output=output.%j      #Send stdout/err to "Example1Out.[jobID]"
#SBATCH --partition=knl

n_cpus=1;

for ((i=0; i<$n_cpus; i++))
do
  echo "Executing taskset -c $i setarch $(uname -m) -R ./a.out $i";
  setarch $(uname -m) -R ./a.out $i;
done

