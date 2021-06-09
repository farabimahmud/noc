import matplotlib.pyplot as plt 
import numpy as np

filename = "latency.log"
n_cpus = 32
def generate_heatmap(l: list):
    arr = np.array(l)
    print(arr.shape)
    plt.imshow(arr, cmap='hot', interpolation='nearest')
    plt.show()  
    plt.savefig('heatmap.png')

    return



if __name__ == '__main__':
    l = [] 
    for n in range(n_cpus):
        l.append([])
    with open(filename) as f:
        for line in f:
            line = line.split(',')
            cpu = int(line[1].strip())
            l[cpu].append(int(line[2].strip()))
    generate_heatmap(l)
   
