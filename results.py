
import os
import glob
import re

benches = [
          'blackscholes',
          'canneal',
          'dedup',
          'facesim',
          'fluidanimate',
          'freqmine',
          'streamcluster',
          'swaptions',
          'x264',
            ]



week_update = "/home/grads/f/farabi/noc/m5out/restore/week-nov-19/"
directory = 'week-nov-19'
stat_files = glob.glob("/home/grads/f/farabi/noc/m5out/restore/week-nov-19/*/stats.txt")
feature_name =  "packet_network_latency_dist |"
for filename in stat_files:
    with open(filename, "r") as f:
        for line in f.readlines():
            if feature_name in line:
                row_data = line.split()
                benchmark_name = filename.replace(week_update,"")
                benchmark_name = benchmark_name[:benchmark_name.index("-")]
                scheme_name = filename.replace("/stats.txt", "")
                scheme_name = scheme_name[scheme_name.rindex("-")+1:]
                print(benchmark_name,",",scheme_name, end=", ")
                for j in range(3,len(row_data),4 ):
                    print(row_data[j], end=", ")
    print()


