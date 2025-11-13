# CSI Collection with openwifi (v1.4.0) on Pop!\_OS Host

This guide walks through bringing up an openwifi SDR board, configuring CSI collection, and visualizing CSI on a Pop!\_OS host PC.

---

## Prerequisites

- Host PC: Pop!\_OS (Linux) with:
  - `ssh`
  - `git`
  - `python3` + `matplotlib`, `numpy` installed
- SDR board running openwifi firmware (reachable at `192.168.10.122`)
- Wired connection from host to SDR board on interface `enx00e04c0a1e09` (adjust interface name if different)

---

## Step 0: Configure the host Ethernet interface

```bash
sudo ip link set enx00e04c0a1e09 up
sudo ip addr flush dev enx00e04c0a1e09
sudo ip addr add 192.168.10.1/24 dev enx00e04c0a1e09
```

**Why:**
You are bringing up the wired NIC that connects directly to the SDR board and assigning it a static IP (`192.168.10.1/24`).

- `ip link set ... up` turns the interface on.
- `ip addr flush` removes any stale IP configuration that might conflict.
- `ip addr add` sets a clean static IP so the host and board are on the same subnet (`192.168.10.x`).

---

## Step 1: SSH into the SDR board

```bash
ssh root@192.168.10.122
```

**Why:**
You need shell access on the SDR board to run all openwifi control scripts (bring up radio, configure monitor mode, load kernel modules, etc.).

---

## Step 2: Go to the openwifi directory on the board

```bash
cd /root/openwifi
```

**Why:**
`/root/openwifi` is the project root on the SDR board. All the helper scripts (`wgd.sh`, `monitor_ch.sh`, `side_ch_ctl`, etc.) and kernel modules live here.

---

## Step 3: Initialize the openwifi network interface

```bash
./wgd.sh
```

**Why:**
This script initializes the openwifi stack and brings up the SDR wireless interface (e.g. `sdr0`). It typically:

- Loads required kernel modules.
- Configures FPGA / radio.
- Sets up the wireless network interface so it can be used in later steps (monitor mode, injection, CSI capture).

---

## Step 4: Put the SDR interface into monitor mode on channel 40

```bash
./monitor_ch.sh sdr0 40
```

**Why:**
You want to capture raw 802.11 frames on a specific channel. This script:

- Puts interface `sdr0` into monitor mode.
- Tunes it to channel 40 (5.2 GHz).
- Ensures that all frames on that channel can be seen by the CSI collection pipeline.

---

## Step 5: Load the side-channel kernel module with 0 equalizer

```bash
insmod ./side_ch.ko num_eq_init=0
```

**Why:**
`side_ch.ko` is the kernel module that exposes side-channel information (including CSI) from the PHY to user space.

- `insmod` inserts this module into the kernel.
- `num_eq_init=0` configures the equalizer behavior (here, disabling or changing default equalizer initialization). This is required for the specific CSI experiment you are running.

---

## Step 6: Configure the side channel to capture CSI only

```bash
./side_ch_ctl wh1h4001
./side_ch_ctl wh7h4433225a
```

**Why:**
`side_ch_ctl` is a userspace control tool for the side-channel module. These hex commands configure:

- What information is sent through the side-channel (e.g. CSI vs other debug info).
- Filters, headers, and formats of the CSI stream.

In this setup, the combination `wh1h4001` and `wh7h4433225a` is used to:

- Enable CSI capture.
- Disable unnecessary fields so only CSI and essential metadata are streamed.

(Details of those hex flags are specific to openwifi’s side-channel protocol.)

---

## Step 7: Unmute the baseband for self receiving

```bash
./sdrctl dev sdr0 set reg xpu 1 1
```

**Why:**
`sdrctl` writes low-level registers on the SDR.
This command:

- Targets device `sdr0`.
- Writes to an internal `xpu` register.
- “Unmutes” or enables the baseband receive path so that transmitted packets can be received by the same device (self-reception), which is necessary to see CSI for injected packets without an external AP.

---

## Step 8: Start streaming side-channel data

```bash
./side_ch_ctl g0
```

**Why:**
This tells the side-channel system to:

- Start “go” mode (streaming).
- Send side-channel data (including CSI) over UDP to the host PC.

At this point, the SDR board is capturing CSI and streaming it out, but you still need to visualize it on the host.

---

## Step 9: On the host PC, clone the openwifi repository

Open a new terminal on your **host PC** (not on the board):

```bash
git clone https://github.com/open-sdr/openwifi.git
cd openwifi
git checkout v1.4.0
```

**Why:**
You need the matching version of the user-space tools for CSI display:

- `git clone` gets the repo.
- `git checkout v1.4.0` ensures you are using the same version as the firmware/scripts on the SDR board, avoiding protocol mismatches between board and visualization code.

---

## Step 10: Run the side-channel visualization script

From the host PC, navigate to the CSI visualization tools:

```bash
cd user_space/side_ch_ctl_src
python3 side_info_display.py 0 waterfall
```

**Why:**
This script:

- Listens for the UDP side-channel packets from the SDR board.
- Parses the CSI and related side information.
- Plots it in real time.

Arguments:

- `0` selects a particular data index or stream (e.g. first antenna / stream index; exact meaning depends on the script).
- `waterfall` enables a waterfall visualization mode so you can see frequency/time evolution of the CSI.

At this point, you should see live graphs updating as packets arrive (assuming the SDR is sending side-channel data and the host can reach it).

---

## Step 11: Inject packets from the SDR board

Open **another** SSH session to the SDR board:

```bash
ssh root@192.168.10.122
```

Then go to the packet injection directory:

```bash
cd /root/openwifi/inject_80211
```

Build and run the injection tool:

```bash
make
./inject_80211 -m n -r 4 -t d -e 8 -b 5a -n 3000 -s 20 -d 10000 sdr0
```

**Why:**
You need a controlled stream of 802.11 packets to generate consistent CSI.

- `make` compiles the `inject_80211` tool.
- `./inject_80211 ... sdr0` sends a burst of packets over `sdr0`.

Typical meanings of flags (exact details depend on the openwifi implementation, but generally):

- `-m n` – use 802.11n modulation/mode.
- `-r 4` – set the MCS/rate index to 4.
- `-t d` – send data-type frames.
- `-e 8` – set payload length or some experiment parameter (e.g. number of OFDM symbols).
- `-b 5a` – use a specific BSSID/identifier (e.g. last byte `0x5a`) for frames.
- `-n 3000` – total number of packets to send.
- `-s 20` – packet size or symbol count (depending on implementation).
- `-d 10000` – delay between packets (e.g. in microseconds).

These injected packets:

- Are transmitted by the SDR.
- Immediately received by the same SDR (self-reception via Step 7).
- Produce CSI that is forwarded via side-channel to your host PC, where `side_info_display.py` plots it.

---

## Expected Result

You should see CSI and side-information plots similar to the official reference:

```markdown
![CSI collection expected results](doc/CSI-collection-expected-results.png)
```

If:

- The host interface is configured correctly (Step 0),
- The SDR is in monitor mode and side-channel is active (Steps 3–8),
- The visualization script is running (Step 10),
- And packets are being injected (Step 11),

then the waterfall and other CSI plots will update live as packets are sent.
