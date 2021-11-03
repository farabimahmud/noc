#!/bin/bash


#SBATCH --job-name=JobExample1       #Set the job name to "JobExample1"
#SBATCH --time=01:00:00              #Set the wall clock limit to 1hr and 30min
#SBATCH --ntasks=1                   #Request 1 task
#SBATCH --mem=2560M                  #Request 2560MB (2.5GB) per node
#SBATCH --output=output.%j      #Send stdout/err to "Example1Out.[jobID]"
#SBATCH --partition=knl

cat /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size
