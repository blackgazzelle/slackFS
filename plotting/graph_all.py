import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

df = pd.read_csv("data.csv")
retrieve_times = dict()
hide_times = dict()
names = []
g = df.groupby(['backend', 'chksum'])
for group in g.groups:
    group_name = f'{group[0]}\n{group[1]}'
    names.append(group_name)
    retrieve_times[group_name] = g.get_group(group).retrieve_time.values
    hide_times[group_name] = g.get_group(group).hide_time.values
 
fig1 = plt.figure(figsize=(15,12))
ax1 = fig1.add_subplot()
ax1.set_title("Overall RetrievalTimes")
ax1.set_xlabel("Number of Runs")
ax1.set_ylabel("Time (s)")
fig2 = plt.figure(figsize=(15,12))
ax2 = fig2.add_subplot()
ax2.set_title("Overall Hide Times")
ax2.set_ylabel("Time (s)")
ax2.set_xlabel("Number of Runs")
for name in names:
    ax1.plot(list(range(len(retrieve_times[name]))), retrieve_times[name], label=name)
    ax2.plot(list(range(len(hide_times[name]))), hide_times[name], label=name)

ax1.set_ylim(bottom=0)
ax1.set_xlim(left=0)
ax2.set_ylim(bottom=0)
ax2.set_xlim(left=0)
ax1.legend(loc='lower right')
ax2.legend(loc='lower right')
fig1.savefig("overall_retrievalTimes.eps", format="eps")
fig1.savefig("overall_retrievalTimes.png")
fig2.savefig("overall_hideTimes.eps", format="eps")
fig2.savefig("overall_hideTimes.png")