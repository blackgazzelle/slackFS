# How does this page work?

- Prev_work: Has the originally proposed slackFS framework without the
 redundancy, as well as, the instructions to run it. This also contains the second version of slackFS
 and links to documentation
- slackFS: This is the python module that runs the hiding, retrieving, and calculating slack functionality
you can modify the amount of fragments and parity fragments are used, which backends are used, and more

## How to use slackFS framework

### Install on Linux

``` bash
# Install packages for bmap and slackFS
sudo apt install make build-essential autoconf automake libtool liberasurecode-dev libjerasure-dev gcc-multilib m4 linuxdoc-tools texlive libisal-dev

# Install poetry for package management  (OPTIONAL)
curl -sSL https://install.python-poetry.org | python3 -

# Install bmap
sudo mkdir -p /usr/local/man/man1
cd bmap && sudo make install

# Install python environemnt or just install requirements.txt
poetry install
poetry shell

# OR
pip install -r requirements.txt
```

### Run slackFS

```bash
./slackFS.pyz -h
```

#### Calculate Slack

```bash
./slackFS.pyz calculate_slack -d /usr -of coverFiles.csv
```

#### Hide disk file

```bash
./slackFS.pyz hide -cf coverFiles.csv -mf map.json -df first50MB.dmg
```

#### Retrieve disk file

```bash
./slackFS.pyz retrieve -mf map.json -of first50MB.dmg.retrieved
```

## Extra Notes

Backends that work with current libraries:

- jerasure_rs_vand
- isa_l_rs_vand
- liberasurecode_rs_vand
- isa_l_rs_cauchy


Backends that don't work (further debugging to be done):

- jerasure_rs_caucy
- flat_xor_hd_3
- flat_xor_hd_4
- shss
