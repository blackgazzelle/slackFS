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

fig1 = plt.figure(figsize=(30,6))
fig2 = plt.figure(figsize=(30,6))
for name in names:
    fig1.plt()