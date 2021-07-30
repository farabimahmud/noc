#!/bin/python3 

import os
import glob
import re
import sys 

#base_dir = "/home/grads/f/farabi/noc/m5out/restore/week-jan-26/"

base_dir = "/home/grads/f/farabi/noc/hpca_results/"
test_case= "rodinia_3"

results = []

def print_feature(stat_files, feature_name, tdir):
    for filename in stat_files:
        with open(filename, "r") as f:
            for line in f.readlines():
                if feature_name in line:
                    row_data = line.split()
                    # print(row_data)
                    benchmark_name = filename.replace(tdir,"")
                    benchmark_name = benchmark_name[1:benchmark_name.index("-")]
                    scheme_name = filename.replace("/stats.txt", "")
                    scheme_name = scheme_name[scheme_name.index("-")+1:]
                    # print(benchmark_name,",",scheme_name, end=",")                   
                    if (feature_name == "packet_network_latency_dist |"):
                        for j in range(3,len(row_data),4 ):
                            print(row_data[j], end=", ")
                    elif (feature_name == "average_packet_network_latency"):
                        print(row_data[1])
                    #elif (row_data[0].startswith("system.switch_cpus")):
                    #    # print("here")
                    elif feature_name == "sim_insts" or feature_name == "sim_ticks":
                        print("{},{},{},{}".format(feature_name,
                            benchmark_name, scheme_name, row_data[1]))
                    elif feature_name == "committedInsts":
                        print("{},{},{},{}".format(feature_name,
                            benchmark_name, scheme_name, row_data[1]))
    print()



tdir = os.path.join(base_dir, test_case)
stat_files = glob.glob( tdir + "/*/stats.txt")
feature_name =  "committedInsts"
# print(stat_files)
print_feature(stat_files, "committedInsts", tdir)
print_feature(stat_files, "sim_insts", tdir)
print_feature(stat_files, "sim_ticks", tdir)
print_feature(stat_files,"packet_network_latency::mean", tdir)
