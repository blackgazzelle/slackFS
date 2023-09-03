# How does this page work?

- Plots: The plots folder has the data, plots, and code used to create the plots 
- Prev_work: Has the originally proposed slackFS framework without the
 redundancy, as well as, the instructions to run it.
- Research_docs: Has different sources used and referenced for erasure coding.
- current_work: contains the code used to test each part of slackFS. It is more
 fragmented than what the ideal framework would look like.

## How to use slackFS framework

### Install

``` bash
# Install packages for bmap and slackFS
sudo apt install make build-essential autoconf automake libtool \
    liberasurecode-dev libjerasure-dev gcc-multilib m4 linuxdoc-tools texlive

# Install bmap
cd bmap && sudo make && sudo make install

# Run bash script to calculate slack space
sudo bashFiles/calc_slack.sh <folder_to_search>

# Run python script
sudo pyCode/processCoverFileList.py <folder_to_search>

# Run slackFS with appropriate args
./slackFS <args>
```
