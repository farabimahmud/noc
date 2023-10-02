#!/bin/bash

./build/X86_MESI_Two_Level/gem5.opt \
    --outdir=baseline \
    --listener-mode=off \
    --debug-flags=ProtocolTrace \
    --redirect-stdout \
    --stdout-file=debug.out \
    --debug-file=debug.out \
    configs/example/se.py \
    --num-cpus=64 \
    --num-dirs=64 \
    --network=garnet2.0 \
    --topology=Mesh_XY \
    --mesh-row=8 \
    --ruby \
    --caches \
    --l2cache \
    --num-l2caches=64 \
    --l1d_size=4kB \
    --mem-size=4GB \
    --bypass=bypass_none \
    --attack-enabled \
    --attack-node=0 \
    --max-hpc=5 \
    --upper-limit=80 \
    --cmd="attack.o" \
    --destination-list=0,63 \
    --target-latency=40 \
    --attack-rate=1 \
    --cpu-type=DerivO3CPU \
    #--fast-forward=9223372036854775807 \
    #--maxinsts=1000000 \

