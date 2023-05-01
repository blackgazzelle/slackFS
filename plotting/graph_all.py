import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

df = pd.read_csv("data.csv")
retrieve_times = list()
hide_times = list()
names = []
g = df.groupby(['backend', 'chksum'])
for group in g.groups:
    group_name = f'{group[0]}\n{group[1]}'
    names.append(group_name)
    retrieve_times.append(np.mean(g.get_group(group).retrieve_time.values))
    hide_times.append(np.mean(g.get_group(group).hide_time.values))

fig1 = plt.figure(figsize=(15,12))
ax1 = fig1.add_subplot()
ax1.set_title("Average RetrievalTimes")
ax1.set_xlabel("Backend Types")
ax1.set_ylabel("Time (s)")
fig2 = plt.figure(figsize=(15,12))
ax2 = fig2.add_subplot()
ax2.set_title("Average Hide Times")
ax2.set_ylabel("Time (s)")
ax2.set_xlabel("Number of Runs")
ax1.bar(names, retrieve_times)
ax2.bar(names, hide_times)
ax1.set_yticks(np.arange(0,max(retrieve_times)+(max(retrieve_times)/4),.05))
ax2.set_yticks(np.arange(0,max(hide_times)+(max(hide_times)/4),.05))

fig1.savefig("overall_retrievalTimes.eps", format="eps")
fig1.savefig("overall_retrievalTimes.png")
fig2.savefig("overall_hideTimes.eps", format="eps")
fig2.savefig("overall_hideTimes.png")