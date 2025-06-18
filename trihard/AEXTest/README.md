# AEX Test

This project measures the number of [AEX-Notify](https://www.intel.com/content/www/us/en/content-details/736463/white-paper-asynchronous-enclave-exit-notify-and-the-edeccssa-user-leaf-function.html) events that occur during a given amount of time.

## Functionalities

The project makes one thread increment a counter and monitors the number of AEX Notify events that occur during this counting. Another thread handles the delay measurement, either by sleeping outside of the enclave (i.e., using a standard `sleep`), or by running code for a while.

## Dependencies

- SGX-enabled hardware (AEX Notify handlers only work in hardware mode, not in simulation)
- AEX Notify availability in kernel: Linux kernel >=6.2
- [Intel SGX-SDK](https://github.com/intel/linux-sgx/tree/sgx_2.24) (tested with version 2.24.100.3)
- `make` apt package
- For figure generation (you can use `make deps`): 
    - Python3, numpy, pandas, and matplotlib packages
    - Latex packages (e.g., for Ubuntu: `sudo apt-get install dvipng texlive-latex-extra texlive-fonts-recommended cm-super`)
- The `msr` kernel module is necessary for some monitoring/attacks: use `sudo apt-get install msr-tools`

## Building AEX Test

To compile the `app` executable:
``` sh
make
```

## Running AEX Test

A makefile recipe `make run` is available with example parameters.

The command line template is the following:
``` sh
./app <SGX_type> <sleep_time> <sleep_type> <verbosity> [<core_main> <core_add>]
```
of using the makefile:
``` sh
make run [SGX=1] [CORE_MAIN=1] [CORE_ADD=2] [SLEEP_TYPE=0] [SLEEP_TIME=10] [VERBOSITY=1]
```
with `<SGX_type>` the SGX hardware type (SGX1 or SGX2); `<sleep_time>` the number of seconds to run the test; `<sleep_type>` whether to measure elapsed time outside the enclave (with value `0`), inside using ocall-based readTSC (with value `1`), an in-enclave readTSC (with value `2`), inside the enclave using a C++ adder (with value `3`), or inside the enclave using an asm adder (with value `4`); `<core_main>` the core id where the main process and thread will be pinned (e.g., for core 2: value `2`); `<core_add>` the core id where the main process and thread will be pinned (e.g., for core 3: value `3`). `verbosity` determines the textual ouput: `0` for no output, `1` for minimal .csv-formated output about AEX events on the counter thread, `2` or more for both AEX monitoring on the counter and timer threads.

The app outputs a .csv-like log of AEX events and at which count value they occured. The last line is the final count before the end of the execution. An example file could be ("//"-comments are absent in the real output):
``` c
idx;count
0;172757 // first AEX occurred after 172757 increments
1;461664
2;751950
3;1051265 // last AEX to occur
4;1354673 // execution finished after 1354673 total increments
```

### Running for experiments

A makefile recipe `make exp` is available that automatically outputs results in a timestamped file `aex-<sleep_time_s>-<timestamp>.csv` (e.g., `aex-30-2024-08-19-10-10-05.csv`):
``` sh
make exp [SGX=1] [CORE_MAIN=1] [CORE_ADD=2] [SLEEP_TYPE=0] [SLEEP_TIME=10]
```
Note that `VERBOSITY` is not an option more this recipe, because subsequent Python scripts rely on the .csv format given by the default value `VERBOSITY=1`.

### Plotting results

Python and shell scripts are available in the `analysis` folder.

#### Measuring AEX counts

All follow the same interface `python <script_name> <results_file_name>.csv [0|1]`, with `results_file_name` the name of a file obtained during an experiment run (e.g., with `make exp` in the previous section), and a parameter 0 or 1 if few or many AEX occured in that experiment (in order to adapt the axis ticks).
The script `gen_plots.sh` in this same `analysis` folder enables to run all 3 scripts on all `.csv` files in the `out` folder, with the same parameter 0 or 1 as the Python scripts, i.e., by running:
``` sh
sh analysis/gen_plots.sh [0|1]
```
Available Python scripts do the following:
- `aex_timeline.py` plots the number of AEX events over the experiment timeframe;
- `aex_difference.py` plots the cumulative number of AEX events which occured *x* (milli)seconds after the previous one;
- `aex_ttpAEX.py` plots for each numbered AEX event the time since the previous AEX event.

#### Measuring counter thread precision

To test the precision of the counter thread (whether there is low/high spread in the number of increments in a given timeframe), `counter_precision_test.sh` is available.
The command line template is:
``` sh
    analysis/counter_precision_test.sh <verbosity> [<sgx_type>*<sleep_type>*<sleep_time_secs>*<n_repeats>]...
```
The first argument, between 0 and 2, runs AEXTest either with the sleep out-/inside the enclave (as before with other scripts.) The following arguments tell how long to sleep each run, and how many run with that sleep time will be executed.
The .csv logs are written in `out/count<verbosity>/count-<timestamp>-<sgx_type>-<sleep_type>-<sleep_time_secs>-<repeat_id>.csv` files.
The format of these files is the same as presented before.

The Python script `plot_count_distributions.py` plots the (cumulative) distributions for given sleeptimes and "in-/out-enclaveness", for each timestamp, i.e., each separate experiment run, using: `plot_count_distributions.py <sgx_type> <sleep_type> <sleep_time_secs>`. Output figures are written to `fig/count-<timestamp>-<sgx_type>-<sleep_type>-<sleep_time_secs>.png` files.

#### Attacking the TSC

The following require the `msr` module to be loaded (using `sudo modprobe msr`).

Directory `tsc_offsetter` contains a .sh script and a .cpp file to manipulate the TSC (by overwriting `MSR 0x10`) on a given core (use `make tsc` to generate the executable from the .cpp file).

To use the .sh script:
``` sh
sh tsc_offsetter/tsc_offsetter.sh <target-core> <read-core> {+|-} <offset_secs> [<loop>]
```
with `<target-core>` the core number where either the monitored or monitoring counter runs; `<read-core>` the core from which to read the TSC's MSR (Model-Specific Register), which can be the same as or different from `<target-core>` (the difference will be in the number potentially caught AEXs); `{+|-}` whether to apply a positive (go in the future) or negative (go in the past) offset by `<offset_secs>` seconds; `<loop>` whether to continually call `wrmsr` to apply the offset (any non-empty argument will trigger the behaviour.)

To use the executable:
``` sh
make tsc
./tsc <target-core> <read-core> <offset_seconds>
```
with `<target-core>` and `<read-core>` the same as before and `<offset_seconds>` a relative integer of how many seconds to offset (negative to go back in time.)

#### Other useful things

To set the core frequency, use:
``` sh
cpupower -c <core-list> frequency-set --{min|max} <MHz-frequency>
```
e.g.:
``` sh
cpupower -c 2-3 frequency-set --min 3500 --max 3500
```
To see the current frequency with the `cpupower`:
``` sh
cpupower -c <core-list> frequency-info
```

The previous approach does not always seem to take effect (seems to either put the min or max core frequency...). 

Alternatively, use `wrmsr` on MSR `MSR_IA32_PERF_CTL` (`0x199`):
``` sh
sudo modprobe msr
sudo wrmsr [-p <core-number>] 0x199 0x<MHz-frequency>
```
You can read the current core frequency with `MSR_IA32_PERF_STATUS` (`0x198`)
``` sh
modprobe msr
sudo rdmsr [-p <core-number>] 0x198
```

Yet another [approach](https://askubuntu.com/questions/1415288/how-to-install-cpupower-on-ubuntu-20-04-with-kernel-5-17), manipulating the system device files directly (see also [this](https://www.kernel.org/doc/html/latest/admin-guide/pm/cpufreq.html?highlight=schedutil#policy-interface-in-sysfs) and [this](https://www.kernel.org/doc/html/latest/admin-guide/pm/cpufreq.html?highlight=schedutil#generic-scaling-governors)):
- for the different infos / changeable parameters:
``` sh
grep . /sys/devices/system/cpu/cpu0/cpufreq/*
```
- setting performance frequency governor
``` sh
grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_available_governors
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```
- checking available frquencies, the current, min, max frequencies, and setting a min/max frequency:
``` sh
grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_available_frequencies
sudo grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_cur_freq
grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq
grep . /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq
echo 3001000 | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_min_freq
echo 3001000 | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_max_freq
```


The relationship between the value given by MSR 0x198 and the one set in 0x199 seems to be that the last 4 hex characters (8 bytes) represent the frequency.
Note that even when using one of the CPU frequency steps provided by `cpupower frequency-info` in `wrmsr` commands, it may be ultimately set at some other step. For example, for one tested machine, using available steps lower than 2GHz works, but above or equal to 2GHz it defaults to 3.5GHz.

To watch interrupts on each core over time:
``` sh
watch -n 1 -d "cat /proc/interrupts"
```

To watch on which cores processes and threads run:
``` sh
watch -n 1 -d "ps -eo psr,tid,pid,comm,%cpu,priority,nice -T | sort -g [| grep <process-name>]"
```