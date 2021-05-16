<!--
Author: Xianjun jiao
SPDX-FileCopyrightText: 2021 UGent
SPDX-License-Identifier: AGPL-3.0-or-later
-->

CSI (Channel Station Information) of WiFi systems is available in some WiFi chips and can be used for environment (people, object, activity) sensing passively and secretly.
 
How could a CSI fuzzer stop unauthorized sensing?

![](./csi-fuzzer-system-before-vs-now.png)

CSI fuzzer implementation principle.

![](./csi-fuzzer-principle.png)

CSI fuzzer in openwifi system architecture and related commands.

![](./csi-fuzzer-implementation.png)

Thanks to the full-duplex capability and CSI extraction feature of openwifi, you can monitor the artificial channel response via [side channel](./csi.md) by Tx-Rx over the air coupling without affecting the normal operation/traffic of openwifi. Before the self-monitoring, the auto-mute during Tx needs to be disabled:

```
./sdrctl dev sdr0 set reg xpu 1 1
```

CSI self-monitoring before fuzzing.

![](./csi-fuzzer-beacon-ant-back-0.jpg)

CSI self-monitoring after  fuzzing command: `csi_fuzzer.sh 1 45 0 13`

![](./csi-fuzzer-beacon-ant-back-1-45-0-13.jpg)

`csi_fuzzer_scan.sh` can scan the c1 and c2 in different styles/modes by calling `csi_fuzzer.sh`.
