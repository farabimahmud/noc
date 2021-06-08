#!/bin/bash
mkdir m5out

../../build/X86_MESI_Two_Level/gem5.opt \
    ../../configs/example/se.py \
    --cpu-type=DerivO3CPU \
    --num-cpus=64 \
    --l1d_size=4kB \
    --num-l2caches=64 \
    --num-dirs=64 \
    --mem-size=4GB \
    --ruby \
    --network=garnet2.0 \
    --topology=Mesh_XY \
    --routing-algorithm=1 \
    --mesh-rows=8 \
    -c ./simple-attack-func > ./m5out/log 2>&1
