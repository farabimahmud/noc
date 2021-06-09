import matplotlib.pyplot as plt 
import numpy as np
import seaborn as sns 
from sklearn.preprocessing import normalize
import pandas as pd
from matplotlib.pyplot import figure
import sys

filename = "latency.csv"
n_cpus = 32
n_sets = 1024

def generate_heatmap(arr : pd.DataFrame):
    # ax = sns.scatterplot(x=l['ADDR_ID'],y=l['CPU_ID'],size=l['LATENCY'], legend=False, sizes=(0,200))

    # plt.show()
    figure(figsize=(8, 6), dpi=600)
    arr = arr.pivot('CPU_ID', 'ADDR_ID','LATENCY')
    ax = sns.heatmap(arr,cmap='rocket')
    # ax = sns.scatterplot(x=arr['ADDR_ID'], y=arr['CPU_ID'], size=arr['LATENCY'], legend= False,sizes=(0,100))
     
    ##    for t in ax.texts:
    ##        if float(t.get_text())>=50:
    ##            t.set_text(t.get_text()) #if the value is greater than 0.4 then I set the text 
    ##        else:
    ##            t.set_text("") # if not it sets an empty text       
    plt.savefig("heatmap.png")
    return

def get_from_log(fname):
    df = pd.DataFrame(columns = ['CPU_ID', 'ADDR_ID', 'LATENCY'])
    i = 0 
    with open(fname) as f:        
        for line in f:
            line = line.split(',')
            cpu = int(line[1].strip())
            df.loc[i]  = [int(line[1].strip()),i % n_sets , int(line[2].strip())]
            i = i+1    
    df.to_csv("latency.csv")



if __name__ == '__main__':

    choice = 1
    if (len(sys.argv) == 2):
        choice = int(sys.argv[1])
    if choice == 2:
        get_from_log("latency.log")
    elif choice == 1:
        print("Reading from latency.csv")
        df = pd.read_csv(filename)
        generate_heatmap(df)
         
