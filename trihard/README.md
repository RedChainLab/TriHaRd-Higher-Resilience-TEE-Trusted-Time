# TriHaRd

This repository contains the source code for the TriHaRd TEE Trusted Time protocol, as well as scripts to run experiments and generate figures.

## Dependencies

- SGX-enabled hardware (AEX Notify handlers only work in hardware mode, not in simulation)
- AEX Notify availability in kernel: Linux kernel >=6.2
- [Intel SGX-SDK](https://github.com/intel/linux-sgx/tree/sgx_2.24) (tested with version 2.24.100.3)
- `make` apt package
- For figure generation (you can use `make deps`): 
    - Python3, numpy, pandas, and matplotlib packages
    - Latex packages (e.g., for Ubuntu: `sudo apt-get install dvipng texlive-latex-extra texlive-fonts-recommended cm-super`)
- The `msr` kernel module is necessary for some monitoring/attacks: use `sudo apt-get install msr-tools`
- The `test-sgx` executable from [SGX-hardware](https://github.com/ayeks/SGX-hardware) is used to automatically define preprocessor options for the executable target, e.g., whether the host supports SGX2, AEX-Notify, both, or neither. That repository should be placed in the [parent directory](..) of this [README](README.md)'s [directory](.).

## Initial setup

To initialize the above dependencies except the Intel SGX SDK (use the [setup.sh](../deployment/deployed/setup.sh) script or deployment tools in the [other directory](../deployment) for that):
```sh
make deps
```

## Compiling and running TriHaRd

```sh
make
time_authority/server <network-RTT> &
make exp [<param>=<value>]...
```
creates a logfile `trihard-(<config-value>-)...-<datetime>.log` in `out/log`

Different parameters can be given to modify the compilation:
- `SINGLE_HOST`: `1` if all three nodes run on the same machine, `0` if each one runs on a different machine (default: `1`). If `0`, the Time Authority IP address can be set in [node/Enclave/ta.h](node/Enclave/ta.h); 
- `TSC_CORE`: the starting core number to place a TSC monitoring thread (default: `2`);
- `NODE_PORT`: the starting port which time service enclaves use to communicate with peers and the Time Authority (default: `12345`);
- `EXPERIMENT_TIME`: the number of minutes to run the experiment *after the first timestamp is obtained by a client*, i.e., shortly after a successful `FREQ` phase (default: `5`).
- `FREQ_OFFSET`: the frequency offset in MHz to `TSC_FREQ` (the OS-measured TSC frequency) for the attacked node (default: `0`);
- `SLEEP_ATTACK_MS`: adds latency in milliseconds on the request (negative values) or the response (positive values) during communications (default: `0`).
- `N_NODES`: the number of time service nodes to run (up to 3, default: `3`).

Some parameters are obtained automatically through scripts, but can be overwritten: `USE_SGX2`, `USE_AEX_NOTIFY`, `TSC_FREQ`.

For experiments using multiple machine, instead of the `exp` recipe, run the executable directly, e.g., using:
```sh
./trihard 12345 2 2.899999 0 0 3 5 [<peer-IP> <peer-port>]... > "out/log/trihard-`date +%Y-%m-%d-%H-%M-%S`.log"
```
i.e., using variable names instead:
```sh
./$(APP_NAME) $(NODE_PORT) $(TSC_CORE) $(TSC_FREQ) $(FREQ_OFFSET) $(SLEEP_ATTACK_MS) $(N_NODES) $(EXPERIMENT_TIME) [<peer-IP> <peer-port>]... > "${LOG_PATH}/trihard-`date +%Y-%m-%d-%H-%M-%S`.log"
```

## Figure generation

To generate figures:
```sh
analysis/analyze_run.sh <logfile-basename>
```
e.g.:
```sh
analysis/analyze_run.sh trihard-2025-03-24-19-33-44
```
Alternatively, to analyze the latest experiment (i.e., the logfile with the latest file creation timestamp, possibly while the experiment is running):
```sh
analysis/analyze_latest.sh
```

To periodically run `analyze_{latest|run}.sh` every `nsecs`:
```sh
analysis/watch_run.sh <nsecs> <logfile-basename> #or
analysis/watch_latest.sh <nsecs>
```
for example to monitor a running experiment using the generated figures.

## Low-interruption environment example

In `/etc/default/grub`:
```sh
GRUB_CMDLINE_LINUX="console=tty0 console=ttyS0,115200n8 console=ttyS1,115200n8 mitigations=off nmi_watchdog=0 nosoftlockup nohz=on nohz_full=2-4,18-20 kthread_cpus=0,16 irqaffinity=0,16 isolcpus=nohz,managed_irq,domain,2-4,18-20 tsc=nowatchdog nowatchdog rcu_nocbs=2-4,18-20 rcu_nocb_poll skew_tick=1 intel_pstate=disable intel_idle.max_cstate=0 processor.max_cstate=0"
```
Followed by:
```sh
sudo update-grub
sudo reboot
```

## Simulating interruptions:
```sh
analysis/sim_interrupts.sh <core> [<proba_in_%oo>-<sleep_time_in_sec>]...
```
e.g., to reproduce interruptions from the Triad paper and log per-core interruption-simulation's start/stop in `out/interrupts.log`:
```sh
export CORE=3; echo "$CORE;`date +%Y-%m-%d-%H-%M-%S`" >> out/interrupts.log; analysis/sim_interrupts.sh $CORE 3400-0.01 3300-0.532 3300-1.5895; echo "$CORE;`date +%Y-%m-%d-%H-%M-%S`">> out/interrupts.log
```
Or use `log_interrupts.sh` that does exactly the above:
```sh
analysis/log_interrupts.sh <core-to-interrupt>
```

### Notes

Provided code is compiled with `-Wall -Werror`:
`#define _LIBCPP_SUPPORT_SGX_SUPPORT_H` needs to be added at `/opt/intel/sgxsdk/include/tlibc/wchar.h:134` in the Intel SGX SDK to avoid double declaration.