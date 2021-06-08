#!/bin/bash


mkdir -p /home/grads/f/farabi/noc/m5out/restore/x86/week-jan-26/blackscholes-16-simmedium-x86-optimized-0
/home/grads/f/farabi/noc/build/X86_MESI_Two_Level/gem5.opt --outdir=/home/grads/f/farabi/noc/m5out/restore/week-jan-26/blackscholes-16-simmedium-x86-optimized-0 /home/grads/f/farabi/noc/configs/example/fs.py --script=/home/grads/f/farabi/noc/scripts_x86/blackscholes_16c_simmedium_ckpts.rcS --kernel=/home/grads/f/farabi/gem5-kernel/x86-parsec/binaries/x86_64-vmlinux-2.6.28.4-smp --disk-image=/home/grads/f/farabi/gem5-kernel/x86-parsec/disks/x86root.img --cpu-type=DerivO3CPU --checkpoint-dir=/home/grads/f/farabi/nuca/m5out/ckpts/blackscholes-16-simmedium-x86 --checkpoint-restore=1 --num-cpus=16 --mem-size=2GB --ruby --num-l2caches=16 --num-dirs=16 --l1d_assoc=8 --l2_assoc=16 --l1i_assoc=4 --network=garnet2.0 --topology=Mesh_XY --mesh-rows=4 --bypass=optimized --flit_jitter_threshold=40 --optimization_rate=0 --maxinsts=1000000 
