
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

week_update = "/home/grads/f/farabi/noc/m5out/restore/week-jan-26/"

week_update = "/home/grads/f/farabi/noc/m5out"


def print_feature(stat_files, feature_name):
    for filename in stat_files:
        with open(filename, "r") as f:
            for line in f.readlines():
                if feature_name in line:
                    row_data = line.split()
                    benchmark_name = filename.replace(week_update,"")
                    benchmark_name = benchmark_name[:benchmark_name.index("-")]
                    scheme_name = filename.replace("/stats.txt", "")
                    scheme_name = scheme_name[scheme_name.rindex("-")+1:]
                    print(benchmark_name,",",scheme_name, end=",")
                    if (feature_name == "packet_network_latency_dist |"):
                        for j in range(3,len(row_data),4 ):
                            print(row_data[j], end=", ")
                    elif (feature_name == "average_packet_network_latency"):
                        print(row_data[1])


        print()



def print_debug_feature(stat_files, feature_name):
    for filename in stat_files:
        with open(filename, "r") as f:
            for line in f.readlines():
                if feature_name in line:
                    row_data = line.split()
                    if (feature_name == "packet_network_latency_dist |") :
                        for j in range(3,len(row_data),4 ):
                            print(row_data[j], end=", ")
                    elif (feature_name == "average_packet_network_latency"):
                        print(row_data[1])
                    elif (feature_name == ".network_latency_dist |"):
                        lindex = row_data[0].index("routers")
                        print(row_data[0][lindex+7:lindex+9], end=", ")
                        for j in range(3, len(row_data), 4):
                            print(row_data[j], end=", ")
                        print()
        print()





stat_files = glob.glob( week_update + "*/stats.txt")
feature_name =  ".network_latency_dist |"
# print(stat_files)
print_debug_feature(stat_files, feature_name)
