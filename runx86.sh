#!/bin/bash

##  ./build/X86_MESI_Two_Level/gem5.opt \
##      --outdir=bypass \
##      --listener-mode=off \
##      --debug-flags=Flitisize \
##      --debug-file=debug.out \
##      configs/example/se.py \
##      --num-cpus=64 \
##      --num-dirs=64 \
##      --network=garnet2.0 \
##      --topology=Mesh_XY \
##      --mesh-row=8 \
##      --maxinsts=1000000 \
##      --ruby \
##      --caches \
##      --l2cache \
##      --num-l2caches=64 \
##      --bypass=bypass_all_out \
##      --rodinia-attack \
##      --attack-enabled \
##      --attack-node=63 \
##      --max-hpc=5 \
##      --upper-limit=80 \
##      --cmd="/home/grads/f/farabi/benchmarks/rodinia_3.0/openmp/backprop/backprop;a.out" \
##      --options="64 63" \
##      --destination-list=0,63 \
##      --target-latency=40 \
##      --attack-rate=1 \
##      --fast-forward=9223372036854775807 \
##  
##  #!/bin/bash
##  #SBATCH --exclude=compute012,compute013,compute014
##  #SBATCH --partition=ada
##  
##  
##  source ~/.bashrc
##  mkdir -p /home/grads/f/farabi/noc/isca_scripts/rodinia_worst_case/srad_v1-jitter_all


/home/grads/f/farabi/noc/build/X86_MESI_Two_Level/gem5.opt \
    --outdir=/home/grads/f/farabi/noc/isca_results/rodinia_worst_case/srad_v1-jitter_all  \
    --redirect-stdout  \
    --redirect-stderr  \
    --stdout-file=/home/grads/f/farabi/noc/isca_results/rodinia_worst_case/srad_v1-jitter_all/stdout.log  \
    --stderr-file=/home/grads/f/farabi/noc/isca_results/rodinia_worst_case/srad_v1-jitter_all/stderr.log  \
    /home/grads/f/farabi/noc/configs/example/se.py \
    --num-cpus=64 --num-dirs=64 --network=garnet2.0 --topology=Mesh_XY \
    --mesh-row=8 --maxinsts=10000000 --ruby --caches --l2cache --num-l2caches=64 \
    --fast-forward=9223372036854775807 \
    --bypass=jitter_all \
    --attack-enabled \
    --max-hpc=5 --upper-limit=50 \
    --cmd=/home/grads/f/farabi/benchmarks/rodinia_3.0/openmp/srad/srad_v1/srad \
    --options="100 0.5 502 458 64" \
    --target-latency=50 \
    --attack-rate=1 \
     --attack-node=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63 \
     --destination-list=0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63 \

