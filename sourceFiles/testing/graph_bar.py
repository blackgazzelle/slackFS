import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

df = pd.read_csv("data.csv")
X = []
retrieve_time = []
hide_time = []
g = df.groupby(['backend', 'chksum'])
for group in g.groups:
    X.append(f"{group[0]}\n{group[1]}")
    retrieve_time.append(g.get_group(group).retrieve_time.mean())
    hide_time.append(g.get_group(group).hide_time.mean())

X_axis = np.arange(len(X))
plt.figure(figsize=(35,10))
plt.bar(X_axis-0.2, hide_time, 0.4, label="Hide Time", align='edge')
plt.bar(X_axis+0.2, retrieve_time, 0.4, label="Retrieve Time", align='edge')
plt.xticks(X_axis, X)
plt.xlabel("Backends/Checksums")
plt.ylabel("Time to run (s)")
plt.title("Average times to execute Hiding and Retrieving files")
plt.legend()
plt.savefig('run3.png')