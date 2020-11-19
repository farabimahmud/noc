#!/bin/bash

for bench in perlbench gcc bwaves mcf cactuBSSN lbm omnetpp wrf xalancbmk \
  x264 cam4 pop2 deepsjeng imagick leela nab exchange2 fotonik3d roms xz
do
  python generate_cpu2017_scripts.py -b $bench
done
