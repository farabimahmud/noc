import pandas as pd
import matplotlib.pyplot as plot


df = pd.read_csv('l2-lat.csv')
# Draw a vertical bar chart
df.plot.bar(x="offset", y="latency", title="Access Latency");
plot.savefig('l2-lat.pdf');
