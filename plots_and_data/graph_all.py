import numpy as np
import matplotlib.pyplot as plt
import matplotlib.colors as mcolors
import pandas as pd


df = pd.read_csv("data.csv")
retrieve_times = list()
hide_times = list()
names = []
g = df.groupby(["backend", "chksum"])
for group in g.groups:
    group_name = f"{group[0]}\n{group[1]}"
    names.append(group_name)
    retrieve_times.append(np.mean(g.get_group(group).retrieve_time.values))
    hide_times.append(np.mean(g.get_group(group).hide_time.values))

fig1 = plt.figure(figsize=(10, 7))
ax1 = fig1.add_subplot()
ax1.set_xlabel("Backend Types")
ax1.set_ylabel("Time (s)")
fig2 = plt.figure(figsize=(10, 7))
ax2 = fig2.add_subplot()
ax2.set_ylabel("Time (s)")
ax2.set_xlabel("Backend Types")
for i in range(len(names)):
    ax1.bar(i, retrieve_times[i], width=0.5)
    ax2.bar(i, hide_times[i], width=0.5)
ax1.set_yticks(np.arange(0, max(retrieve_times) + (max(retrieve_times) / 4),
                         0.05))
ax2.set_yticks(np.arange(0, max(hide_times) + (max(hide_times) / 4), 0.05))
fig1.tight_layout()
fig1.legend(names, fontsize=8)
fig1.savefig("overall_retrievalTimes.eps", format="eps")
fig1.savefig("overall_retrievalTimes.png")
fig2.tight_layout()
fig2.legend(names, fontsize=8)
fig2.savefig("overall_hideTimes.eps", format="eps")
fig2.savefig("overall_hideTimes.png")
