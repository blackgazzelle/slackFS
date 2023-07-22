import matplotlib.pyplot as plt
import pandas as pd
import numpy as np


def plot(times, names, time_type):
    title = time_type + " "
    i = 0
    for name in names:
        if i < len(names) - 1:
            title += name.replace("\n", " ") + " vs "
        else:
            title += name.replace("\n", " ")
        i += 1
    fig1 = plt.figure(figsize=(10, 7))
    ax1 = fig1.add_subplot()
    ax1.set_xlabel("Number of Runs")
    ax1.set_ylabel("Time (s)")

    smallest = []
    largest = []
    for name in names:
        ax1.plot(
            np.append(list(range(0, len(times[name]), 5)), [50]),
            np.append(times[name][::5], times[name][-1]),
            label=name,
        )
        # get minimum num
        smallest.append(min(times[name]))
        largest.append(max(times[name]))
    bot = min(smallest)
    top = max(largest)

    ax1.set_ylim(bottom=bot - (bot / 4), top=top + (top / 4))
    ax1.set_xlim(left=1)
    ax1.legend(loc="upper right")
    fig1.tight_layout()
    fig1.savefig(f"{title}.eps", format="eps")
    fig1.savefig(f"{title}.png")


def plot_alternate(times, names, time_type):
    i = 0
    title = time_type[i] + " "
    for name in names:
        if i < len(names) - 1:
            i += 1
            title += name.replace("\n", " ") + " vs " + time_type[i] + " "
        else:
            i += 1
            title += name.replace("\n", " ")

    fig1 = plt.figure(figsize=(10, 7))
    ax1 = fig1.add_subplot()
    ax1.set_xlabel("Number of Runs")
    ax1.set_ylabel("Time (s)")

    smallest = []
    largest = []
    for i, name in enumerate(names):
        ax1.plot(
            np.append(list(range(0, len(times[i][name]), 5)), [50]),
            np.append(times[i][name][::5], times[i][name][-1]),
            label=f"{time_type[i]} {name}",
        )
        # get minimum num
        smallest.append(min(times[i][name]))
        largest.append(max(times[i][name]))
    bot = min(smallest)
    top = max(largest)
    ax1.set_ylim(bottom=bot - (bot / 4), top=top + (top / 4))
    ax1.set_xlim(left=1)
    ax1.legend(loc="upper right")
    fig1.tight_layout()
    fig1.savefig(f"{title}.eps", format="eps")
    fig1.savefig(f"{title}.png")


def main():
    df = pd.read_csv("16f_8p.csv")
    retrieve_times = dict()
    hide_times = dict()
    names = []
    g = df.groupby(["backend", "chksum"])
    for group in g.groups:
        group_name = f"{group[0]}\n{group[1]}"
        names.append(group_name)
        retrieve_times[group_name] = g.get_group(group).retrieve_time.values
        hide_times[group_name] = g.get_group(group).hide_time.values

    graph_sets = list()
    used = list()
    for group in g.groups:
        for other_group in g.groups:
            if not group == other_group:
                if (
                    not (group, other_group) in used
                    and not (other_group, group) in used
                ):
                    graph_sets.append(
                        [
                            f"{group[0]}\n{group[1]}",
                            f"{other_group[0]}\n{other_group[1]}",
                        ]
                    )
                    used.append((group, other_group))
    graph_sets.append(
        [
            "BACKEND_NULL\nCHKSUM_MD5",
            "BACKEND_NULL\nCHKSUM_NONE",
            "BACKEND_NULL\nCHKSUM_CRC32",
        ]
    )

    for graph_set in graph_sets:
        plot(retrieve_times, graph_set, "Retrieval")
        plot(hide_times, graph_set, "Hiding")

    alternate_sets = {
        ("JERASURE_RS_VAND\nCHKSUM_NONE", "JERASURE_RS_VAND\nCHKSUM_CRC32"): "H:R",
    }
    for group in g.groups:
        alternate_sets[f"{group[0]}\n{group[1]}"] = "H:R"

    for key in alternate_sets:
        if alternate_sets[key] == "H:R":
            plot_alternate([hide_times, retrieve_times], key, ["Hiding", "Retrieval"])


if __name__ == "__main__":
    main()
