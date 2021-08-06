#!/bin/bash

./build/X86_MESI_Two_Level/gem5.opt \
    --listener-mode=off \
    --outdir=$PWD/bypass \
    --debug-flag=Vanilla \
    --debug-file=debug.out \
    configs/example/se.py \
    --num-cpus=64 \
    --num-dirs=64 \
    --network=garnet2.0 \
    --topology=Mesh_XY \
    --mesh-rows=8 \
    --ruby \
    --l2cache \
    --num-l2caches=64 \
    --caches \
    -c /home/grads/f/farabi/benchmarks/rodinia_3.0/openmp/hotspot/hotspot \
    -o "512 512 2 64 /home/grads/f/farabi/benchmarks/rodinia_3.0/data/hotspot/temp_512 /home/grads/f/farabi/benchmarks/rodinia_3.0/data/hotspot/power_512" \
    --bypass=bypass_all \
    --attack-enabled \
    --attack-node=0 \
    --attack-rate=0.1 \
    --max-hpc=5 \
    --upper-limit=80 \
    --fast-forward=9223372036854775807 \
    --destination-list=8,53 \
    --maxinsts=100000 \
