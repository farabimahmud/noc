
import os
import glob
import sys

import matplotlib.pyplot as plt
from scipy.ndimage import gaussian_filter1d

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
week_update = "/home/farabi/noc/"
# week_update = "/home/grads/f/farabi/noc/"


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
                    elif (feature_name == ".regular_network_latency_dist |"):
                        # print(row_data)
                        lindex = row_data[0].index("routers")
                        print("Router", row_data[0][lindex+7:lindex+9], 
                                end=", ")
                        for j in range(3, len(row_data), 4):
                            print(row_data[j], end=", ")
                        print()
                    elif (feature_name == ".attack_network_latency_dist |"):
                        # print(row_data)
                        lindex = row_data[0].index("routers")
                        print("Router", row_data[0][lindex+7:lindex+9], 
                                end=", ")
                        for j in range(3, len(row_data), 4):
                            print(row_data[j], end=", ")
                        print()
        print()

def plot_debug_feature(stat_files, feature_name):

    for filename in stat_files:
        with open(filename, "r") as f:
            for line in f.readlines():
                if feature_name in line:
                    x = range(0,51,2)
                    y = []
                    plot_file = ""

                    row_data = line.split()
                    if (row_data[3][:-1] == ''):
                        continue
                    if (feature_name == "packet_network_latency_dist |") :
                        for j in range(3,len(row_data),4 ):
                            print(row_data[j], end=", ")
                    elif (feature_name == "average_packet_network_latency"):
                        print(row_data[1])
                    elif feature_name.endswith("network_latency_dist |"):
                        lindex = row_data[0].index("routers")
                        plot_file = row_data[0][lindex+7:lindex+9]
                        print(row_data[0][lindex+7:lindex+9], end=", ")
                        for j in range(3, len(row_data), 4):
                            y.append(float(row_data[j][:-1]))

                    # y = gaussian_filter1d(y,sigma=3)
                    plt.bar(x,y)
                    plt.ylim(0,100)
                    plt.ylabel("Frequency")
                    plt.xlabel("Cycles")
                    plt.title("Source {} Destination {}".format(0,
                        int(plot_file)))
                    plt.savefig("plots/"+plot_file+".png")
                    plt.cla()
        print()



stat_files = glob.glob( week_update + "*/stats.txt")
feature_name =  ".network_latency_dist |"
feature_names =  [
    ".network_latency_dist |",
    ".attack_network_latency_dist |",
    ".regular_network_latency_dist |"
    ]

feature_names =  [
    ".regular_network_latency_dist |",
    # ".attack_network_latency_dist |",
    # ".network_latency_dist |",

    ]
print(stat_files)
try:
    os.mkdir("plots")
except:
    pass

if (len(sys.argv) ==2):
    print (sys.argv)
    if (sys.argv[1] )== '1':
        feature_names = [".attack_network_latency_dist |"]
    elif (sys.argv[1]) == '2':
        feature_names = [".regular_network_latency_dist |"]

# plot_debug_feature(stat_files, feature_name)
for f in feature_names:
    print(f)
    plot_debug_feature(stat_files, f)
