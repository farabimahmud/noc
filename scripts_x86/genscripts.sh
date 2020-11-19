#!/bin/sh

for bench in blackscholes bodytrack canneal dedup facesim ferret fluidanimate freqmine streamcluster swaptions x264
do
  ./writescripts.pl $bench 16 --ckpts
done
