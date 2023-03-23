import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

def plot(times, names, time_type):
    title = time_type + " "
    i = 0
    for name in names:
        if i < len(names)-1:
            title += name.replace('\n', ' ') + " vs "
        else:
            title += name.replace('\n', ' ')
        i += 1
    fig1 = plt.figure(figsize=(15,12))
    ax1 = fig1.add_subplot()
    ax1.set_title(title)
    ax1.set_xlabel("Number of Runs")
    ax1.set_ylabel("Time (s)")

    for name in names:
        ax1.plot(list(range(len(times[name]))), times[name], label=name)

    ax1.set_ylim(bottom=0)
    ax1.set_xlim(left=0)
    ax1.legend(loc='lower right')
    fig1.savefig(f"{title}.eps", format="eps")
    fig1.savefig(f"{title}.png")

def main():
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
    
    graph_sets = [
                ['BACKEND_NULL\nCHKSUM_CRC32', 'JERASURE_RS_VAND\nCHKSUM_CRC32'],
                ['BACKEND_NULL\nCHKSUM_NONE', 'JERASURE_RS_VAND\nCHKSUM_NONE'], 
                ['BACKEND_NULL\nCHKSUM_MD5', 'BACKEND_NULL\nCHKSUM_NONE', 'BACKEND_NULL\nCHKSUM_CRC32'], 
                ['JERASURE_RS_VAND\nCHKSUM_NONE', 'JERASURE_RS_VAND\nCHKSUM_CRC32']
            ]
    for graph_set in graph_sets:
        plot(retrieve_times, graph_set, "Retrieval")
        plot(hide_times, graph_set, "Hiding")

if __name__ == '__main__':
    main()