import matplotlib.pyplot as plt
import math
import sys
import pandas as pd
import numpy as np

params = {'text.usetex' : True,
          'font.size' : 10,
            'font.family' : 'serif',
            'font.serif' : 'Computer Modern Roman',
          }
plt.rcParams.update(params) 

# x-axis params
MAJOR_TICKS=600 #240 #600 #60
MINOR_TICKS=100 #60 #100 #10
LOW_INTERRUPTS = False
TSC_FREQ=2.899999e9

# Files in out/count directory are in the naming format count-<timestamp>-<sgx_type>-<sleep_type>-<sleep_time_secs>-<repeat_id>.csv
# Get <sleep_type> <sleep_time_secs> from command line arguments
vlines=[]
if len(sys.argv) > 1:
  file = sys.argv[1]
if len(sys.argv) > 2:
  vlines=[int(x) for x in sys.argv[2].split(",")]

#print(files_timestamp)
try:
  with open(f'out/ts/{file}-ts-node.log', 'r') as f:
    df = pd.read_csv(f, header=None, sep=' ')
except Exception as e:
  print(f'Error reading {file}-ts-node.log: {e}\n')
try:
  with open(f'out/ts/{file}-aex.log', 'r') as f:
    aex_df = pd.read_csv(f, header=None, sep=' ')
except Exception as e:
  print(f'Error reading {file}-aex: {e}\n')
try:
  with open(f'out/ts/{file}-ut-node.log', 'r') as f:
    ut_node_df = pd.read_csv(f, header=None, sep=' ')
except Exception as e:
  print(f'Error reading {file}-ut-node.log: {e}\n')
try:
  with open(f'out/ts/{file}-ut-ta.log', 'r') as f:
    ut_ta_df = pd.read_csv(f, header=None, sep=' ')
except Exception as e:
  print(f'Error reading {file}-ut-ta.log: {e}\n')
try:
  with open(f'out/ts/{file}-states.log', 'r') as f:
    states_df = pd.read_csv(f, header=None, sep=' ')
except Exception as e:
  print(f'Error reading {file}-states.log: {e}\n')
try:
  with open(f'out/ts/{file}-clock-states.log', 'r') as f:
    clock_states_df = pd.read_csv(f, header=None, sep=' ')
except Exception as e:
  print(f'Error reading {file}-clock-states.log: {e}\n')
try:
  with open(f'out/ts/{file}-node-states.log', 'r') as f:
    node_states_df = pd.read_csv(f, header=None, sep=' ')
except Exception as e:
  print(f'Error reading {file}-node-states.log: {e}\n')
# print(df)
# print(aex_df)
# print(ut_node_df)
# print(ut_ta_df)
# print(states_df)

FORMAT="%m/%d/%y %H:%M:%S.%f"

aex_df.drop(columns=[0,3], inplace=True)
aex_df.columns = ['ID','type', 'date', "time","TZ"]
aex_df["datetime"]=pd.to_datetime(aex_df["date"]+" "+aex_df["time"],format=FORMAT)
# print(aex_df)

ut_ta_df.drop(columns=[0,3], inplace=True)
ut_ta_df.columns = ['ID','type', 'date', "time","TZ"]
ut_ta_df["datetime"]=pd.to_datetime(ut_ta_df["date"]+" "+ut_ta_df["time"],format=FORMAT)
# print(ut_ta_df)

ut_node_df.drop(columns=[0,3], inplace=True)
ut_node_df.columns = ['ID','type', 'date', "time","TZ"]
ut_node_df["datetime"]=pd.to_datetime(ut_node_df["date"]+" "+ut_node_df["time"],format=FORMAT)
# print(ut_node_df)

states_df.drop(columns=[0,3], inplace=True)
states_df.columns = ['ID','type', 'date', "time","TZ"]
states_df["datetime"]=pd.to_datetime(states_df["date"]+" "+states_df["time"],format=FORMAT)
states_df["type"].replace({"OK": 0, "Tainted": 1, "FullCalib":2}, inplace=True)
# print(states_df)

clock_states_df.drop(columns=[0,3], inplace=True)
clock_states_df.columns = ['ID','type', 'date', "time","TZ"]
clock_states_df["datetime"]=pd.to_datetime(clock_states_df["date"]+" "+clock_states_df["time"],format=FORMAT)
clock_states_df["type"].replace({"TAInconsistent": 0, "TAConsistent": 1}, inplace=True)
# print(clock_states_df)

node_states_df.drop(columns=[0,3], inplace=True)
node_states_df.columns = ['ID','type', 'date', "time","TZ"]
node_states_df["datetime"]=pd.to_datetime(node_states_df["date"]+" "+node_states_df["time"],format=FORMAT)
node_states_df["type"].replace({"Sync": 1, "Freq":0}, inplace=True)
# print(node_states_df)

df.drop(columns=[0,3], inplace=True)
df.columns = ['ID','type', 'date', "time","TZ", "cycles"]
node_ts = df[df['type'] == 'Node'].reset_index(drop=True)
ref_ts = df[df['type'] == 'Ref.'].reset_index(drop=True)
merged=node_ts.join(ref_ts, lsuffix='_node', rsuffix='_ref')
merged.drop(columns=['type_node', 'type_ref', 'TZ_node', 'TZ_ref'], inplace=True)
merged=merged[~merged["time_node"].str.contains("-")].dropna()
merged["datetime_node"]=pd.to_datetime(merged["date_node"]+" "+merged["time_node"],format=FORMAT)
merged["datetime_ref"]=pd.to_datetime(merged["date_ref"]+" "+merged["time_ref"],format=FORMAT)
merged.drop(columns=["date_node", "time_node", "date_ref", "time_ref"], inplace=True)
merged["drift"]=(merged["datetime_node"]-merged["datetime_ref"])/pd.Timedelta('1ms')
merged["cycles_node"]=merged["cycles_node"].astype(float)
# merged["cycles_node"]=merged["cycles_node"] / TSC_FREQ
merged["cycles_ref"]=merged["cycles_ref"].astype(float)
# merged["cycles_ref"]=merged["cycles_ref"] / TSC_FREQ

ref_datetime=min(merged["datetime_ref"].min(),aex_df["datetime"].min(),ut_ta_df["datetime"].min(),ut_node_df["datetime"].min())

merged["datetime_ref"]=(merged["datetime_ref"]-ref_datetime)/pd.Timedelta('1s')
aex_df["datetime"]=(aex_df["datetime"]-ref_datetime)/pd.Timedelta('1s')
ut_ta_df["datetime"]=(ut_ta_df["datetime"]-ref_datetime)/pd.Timedelta('1s')
ut_node_df["datetime"]=(ut_node_df["datetime"]-ref_datetime)/pd.Timedelta('1s')
states_df["datetime"]=(states_df["datetime"]-ref_datetime)/pd.Timedelta('1s')
clock_states_df["datetime"]=(clock_states_df["datetime"]-ref_datetime)/pd.Timedelta('1s')
node_states_df["datetime"]=(node_states_df["datetime"]-ref_datetime)/pd.Timedelta('1s')

# Filter data to keep only rows where datetime is greater than 100 seconds, i.e., after switch from FREQ->SYNC state
# merged = merged[merged["datetime_ref"] < 600]
# aex_df = aex_df[aex_df["datetime"] > 100]
# ut_ta_df = ut_ta_df[ut_ta_df["datetime"] > 100]
# ut_node_df = ut_node_df[ut_node_df["datetime"] > 100]
states_df = states_df[(states_df["datetime"] > 150) & (states_df["datetime"] < 600)]
# clock_states_df = clock_states_df[clock_states_df["datetime"] > 100]
# node_states_df = node_states_df[node_states_df["datetime"] > 100]

mem_aex_df=aex_df.copy()

df_list=[aex_df, ut_ta_df, ut_node_df]
for idx, a_df in enumerate(df_list):
# Add a row per node in aex_df with the datetime as merged["datetime_ref"].max() + 1 second
  unique_ids = a_df['ID'].unique()
  new_rows = pd.DataFrame({
    'ID': unique_ids,
    'datetime': [merged["datetime_ref"].max() + 1] * len(unique_ids)
  })
  a_df = pd.concat([a_df, new_rows], ignore_index=True)
  df_list[idx] = a_df
aex_df, ut_ta_df, ut_node_df = df_list

# Duplicate the last record for each node in states_df with a new row at datetime of merged["datetime_ref"].max + 1
unique_ids = states_df['ID'].unique()
new_rows = states_df.groupby('ID').tail(1).copy()
new_rows['datetime'] = merged["datetime_ref"].max() + 1
states_df = pd.concat([states_df, new_rows], ignore_index=True)

# Duplicate the last record for each node in clock_states_df with a new row at datetime of merged["datetime_ref"].max + 1
unique_ids = clock_states_df['ID'].unique()
new_rows = clock_states_df.groupby('ID').tail(1).copy()
new_rows['datetime'] = merged["datetime_ref"].max() + 1
clock_states_df = pd.concat([clock_states_df, new_rows], ignore_index=True)

# Duplicate the last record for each node in node_states_df with a new row at datetime of merged["datetime_ref"].max + 1
unique_ids = node_states_df['ID'].unique()
new_rows = node_states_df.groupby('ID').tail(1).copy()
new_rows['datetime'] = merged["datetime_ref"].max() + 1
node_states_df = pd.concat([node_states_df, new_rows], ignore_index=True)

# print(node_ts, ref_ts, merged)
NB_FIGS=9
fig_ax = [plt.subplots(figsize=(4.5,1.5)) for _ in range(NB_FIGS)]

fig, ax = zip(*fig_ax)

colors=["black","tab:orange","tab:blue"]
linestyles=[":","--","-"]
for (idx, group), color in zip(enumerate(merged.groupby("ID_node")), colors):
  ax[0].plot(group[1]["datetime_ref"], group[1]["drift"], marker='+', markersize=3, linestyle="-", linewidth=0.5, label=f"Node {1+int(group[0][:-2])%12345}", color=color,
             #zorder=4-idx
             )

for (idx, group), color, linestyle in zip(enumerate(aex_df.groupby("ID")), colors, linestyles):
  ax[1].step(group[1]["datetime"], np.cumsum(np.ones(len(group[1]["datetime"]))), linestyle=linestyle, label=f"Node {1+int(group[0][:-2])%12345}", color=color)

for (idx, group), color, linestyle in zip(enumerate(ut_ta_df.groupby("ID")), colors, linestyles):
  ax[2].step(group[1]["datetime"], np.cumsum(np.ones(len(group[1]["datetime"]))), linestyle=linestyle, label=f"Node {1+int(group[0][:-2])%12345}", color=color, where='post')

for (idx, group), color, linestyle in zip(enumerate(ut_node_df.groupby("ID")), colors, linestyles):
  ax[3].step(group[1]["datetime"], np.cumsum(np.ones(len(group[1]["datetime"]))), linestyle=linestyle, label=f"Node {1+int(group[0][:-2])%12345}", color=color, where='post')

for (idx, group), color, linestyle in zip(enumerate(states_df.groupby("ID")), colors, linestyles):
  ax[4].step(group[1]["datetime"], group[1]["type"], linestyle=linestyle, label=f"Node {1+int(group[0][:-2])%12345}", color=color, where='post', 
            #  zorder=4-idx
            )

for (idx, group), color, linestyle in zip(enumerate(clock_states_df.groupby("ID")), colors, linestyles):
  ax[5].step(group[1]["datetime"], group[1]["type"], linestyle=linestyle, label=f"Node {1+int(group[0][:-2])%12345}", color=color, where='post', 
            #  zorder=4-idx
            )

for (idx, group), color, linestyle in zip(enumerate(node_states_df.groupby("ID")), colors, linestyles):
  ax[6].step(group[1]["datetime"], group[1]["type"], linestyle=linestyle, label=f"Node {1+int(group[0][:-2])%12345}", color=color, where='post', 
            #  zorder=4-idx
            )

for (idx, group), color in zip(enumerate(merged.groupby("ID_node")), colors):
  ax[7].plot(group[1]["datetime_ref"].iloc[1:], group[1]["cycles_node"].iloc[1:], marker='o', markersize=3, linestyle="None", color=color, label=f"Node {1+int(group[0][:-2])%12345} cycles",
             #zorder=4-idx
             )

for (idx, group), color in zip(enumerate(merged.groupby("ID_node")), colors):
  ax[8].plot(group[1]["datetime_ref"].iloc[1:], group[1]["cycles_ref"].iloc[1:], marker='o', markersize=3, linestyle="None", color=color, label=f"Node {1+int(group[0][:-2])%12345} cycles",
             #zorder=4-idx
             )

for a in ax:
  a.grid(True)
  a.grid(which='minor', linestyle=':', linewidth='0.5')
  a.grid(which='major', linestyle='-', linewidth='0.5')
  a.minorticks_on()

ax[3].set_ylim(-100)

ax[4].grid(axis="x", which='minor', linestyle=':', linewidth='0.5')
ax[4].set_yticks([0,1,2])
ax[4].set_yticklabels(["OK","Tainted","FullCalib"])
ax[4].minorticks_on()

ax[5].grid(axis="x", which='minor', linestyle=':', linewidth='0.5')
ax[5].set_yticks([0,1])
ax[5].set_yticklabels(["TA-Inconsistent","TA-Consistent"])
ax[5].minorticks_on()

ax[6].grid(axis="x", which='minor', linestyle=':', linewidth='0.5')
ax[6].set_yticks([0,1])
ax[6].set_yticklabels(["FREQ","SYNC"])
ax[6].minorticks_on()

ax[7].set_yscale('log')
# ax[7].set_ylim(1e4, 1e5)
ax[8].set_yscale('log')
# ax[8].set_ylim(1e-6, 1e-3)

for axis in ax:
  axis.set_xlabel('Reference time (s)')
  axis.set_xticks(np.arange(0, min(3601,math.ceil(merged["datetime_ref"].max()+1)), MAJOR_TICKS))
  axis.set_xlim(0, min(3601,math.ceil(merged["datetime_ref"].max())))
  axis.set_xticks(np.arange(0, min(3601,math.ceil(merged["datetime_ref"].max()+1)), MINOR_TICKS), minor=True)

ax[0].set_ylabel('Drift (ms)')
ax[1].set_ylabel('AEX count')
ax[2].set_ylabel('Message count to TA')
ax[3].set_ylabel('Peer response count')
ax[4].set_ylabel('AEX state')
ax[5].set_ylabel('Clock state')
ax[6].set_ylabel('Node state')
ax[7].set_ylabel('Node cycles')
ax[8].set_ylabel('Ref. cycles')
# ax[0].set_xlim(0)
ax[2].legend(loc='lower right', fontsize='small')
ax[1].legend(loc='lower right', fontsize='small')
# ax[0].legend(loc='lower left', fontsize='small')

for vline in vlines:
  for a in ax:
    a.axvline(x=vline, color='r', linestyle='--', zorder=100)

suffixes=["drift","aex","ut-ta","ut-node","states","clock-states","node-states","node-cycles","ref-cycles"]
for f,suffix in zip(fig,suffixes):
  f.savefig(f'fig/{file}-{suffix}.pdf', bbox_inches='tight', dpi=1200)

# Filter states_df to keep data with datetime higher than the earliest row with "type" == 1 for each ID
earliest_tainted = states_df[states_df["type"] == 1].groupby("ID")["datetime"].min()
states_df = states_df.merge(earliest_tainted, on="ID", how="left", suffixes=("", "_earliest"))
states_df = states_df[states_df["datetime"] > states_df["datetime_earliest"]].drop(columns=["datetime_earliest"])

def compute_state_durations(states_df, state_value):
  state_df = states_df.copy()
  state_df["type"] = states_df["type"].replace({0: 0, 1: 0, 2: 0, state_value: 1})
  state_df["duration"] = state_df.groupby("ID")["datetime"].diff().fillna(0)
  state_df["duration"] = state_df.groupby("ID")["duration"].shift(-1).fillna(0)
  state_df = state_df[state_df["type"] == 1]
  state_durations = state_df.groupby("ID")["duration"].sum()
  return state_durations

ok_durations = compute_state_durations(states_df, 0)
tainted_durations = compute_state_durations(states_df, 1)
fullcalib_durations = compute_state_durations(states_df, 2)

# Merge the three state durations into a single DataFrame
state_durations_df = pd.DataFrame({
  'OK': ok_durations,
  'Tainted': tainted_durations,
  'FullCalib': fullcalib_durations
}).fillna(0)
state_durations_df["ID"]=state_durations_df.index.str[:-2]
state_durations_df["ID"]=(state_durations_df["ID"].astype(int)+1)%12345
state_durations_df["ID"]=state_durations_df["ID"].astype(str)
state_durations_df.set_index("ID", inplace=True)

# Plot the state durations
fig2, ax2 = plt.subplots(figsize=(4.5, 2.5))
state_durations_df_normalized = state_durations_df.div(state_durations_df.sum(axis=1), axis=0) * 100

bar_width = 0.2
index = np.arange(len(state_durations_df_normalized))

hatching = ['/', 'xx', '\\', 'o']

ax2.bar(index + bar_width/2, state_durations_df_normalized['OK'], bar_width, color='tab:blue', label='OK', edgecolor='black', hatch=hatching[0])
ax2.bar(index + 3*bar_width/2, state_durations_df_normalized['Tainted'], bar_width, color='tab:orange', label='Tainted TS', edgecolor='black', hatch=hatching[1])
ax2.bar(index + 5*bar_width/2, state_durations_df_normalized['FullCalib'], bar_width, color='tab:red', label='Full calib.', edgecolor='black', hatch=hatching[2])

ax2.set_xlabel('Node ID')
ax2.set_xticks(index + 1.5 * bar_width)
ax2.set_xticklabels(state_durations_df_normalized.index)
ax2.set_ylabel('Duration (\\%)')
ax2.set_ylim(0.0001, 100)
ax2.set_yscale('log')
ax2.grid(axis='y', linestyle='-', linewidth='0.5')
ax2.grid(axis='y', which='minor', linestyle=':', linewidth='0.5')
ax2.minorticks_on()
handles, labels = ax2.get_legend_handles_labels()
ax2.legend(handles, labels, title='AEX state', loc='upper center', fontsize='small', ncol=3, handleheight=1.5, bbox_to_anchor=(0.425, 1.3),handletextpad=0.5)
fig2.savefig(f'fig/{file}-state-durations.pdf', bbox_inches='tight', dpi=1200)

ta_consistent_durations = compute_state_durations(clock_states_df, 1)
ta_inconsistent_durations = compute_state_durations(clock_states_df, 0)

# Merge the three state durations into a single DataFrame
clock_state_durations_df = pd.DataFrame({
  'TAConsistent': ta_consistent_durations,
  'TAInconsistent': ta_inconsistent_durations
}).fillna(0)
clock_state_durations_df["ID"] = clock_state_durations_df.index.str[:-2]
clock_state_durations_df["ID"] = (clock_state_durations_df["ID"].astype(int) + 1) % 12345
clock_state_durations_df["ID"] = clock_state_durations_df["ID"].astype(str)
clock_state_durations_df.set_index("ID", inplace=True)

# Plot the clock state durations
fig2, ax2 = plt.subplots(figsize=(4.5, 2.5))
clock_state_durations_df_normalized = clock_state_durations_df.div(clock_state_durations_df.sum(axis=1), axis=0) * 100

bar_width = 0.2
index = np.arange(len(clock_state_durations_df_normalized))

hatching = ['xx', 'o']

ax2.bar(index + bar_width, clock_state_durations_df_normalized['TAConsistent'], bar_width, color='tab:orange', label='TA-Consistent', edgecolor='black', hatch=hatching[0])
ax2.bar(index + 2*bar_width, clock_state_durations_df_normalized['TAInconsistent'], bar_width, color='tab:red', label='TA-Inconsistent', edgecolor='black', hatch=hatching[1])

ax2.set_xlabel('Node ID')
ax2.set_xticks(index + 1.5 * bar_width)
ax2.set_xticklabels(clock_state_durations_df_normalized.index)
ax2.set_ylabel('Duration (\\%)')
ax2.set_ylim(0.01, 100)
ax2.set_yscale('log')
ax2.grid(axis='y', linestyle='-', linewidth='0.5')
ax2.grid(axis='y', which='minor', linestyle=':', linewidth='0.5')
ax2.minorticks_on()
handles, labels = ax2.get_legend_handles_labels()
ax2.legend(handles, labels, title='Clock state', loc='upper center', fontsize='small', ncol=3, handleheight=1.5, bbox_to_anchor=(0.425, 1.3), handletextpad=0.5)
fig2.savefig(f'fig/{file}-clock-state-durations.pdf', bbox_inches='tight', dpi=1200)

sync_durations = compute_state_durations(node_states_df, 0)
spike_durations = compute_state_durations(node_states_df, 1)
freq_durations = compute_state_durations(node_states_df, 2)

# Merge the three state durations into a single DataFrame
node_state_durations_df = pd.DataFrame({
  'Sync': ok_durations,
  'Freq': freq_durations
}).fillna(0)
node_state_durations_df["ID"] = node_state_durations_df.index.str[:-2]
node_state_durations_df["ID"] = (node_state_durations_df["ID"].astype(int) + 1) % 12345
node_state_durations_df["ID"] = node_state_durations_df["ID"].astype(str)
node_state_durations_df.set_index("ID", inplace=True)

# Plot the node state durations
fig2, ax2 = plt.subplots(figsize=(4.5, 2.5))
node_state_durations_df_normalized = node_state_durations_df.div(node_state_durations_df.sum(axis=1), axis=0) * 100

bar_width = 0.2
index = np.arange(len(node_state_durations_df_normalized))

hatching = ['/', 'xx', '\\', 'o']

ax2.bar(index + bar_width, node_state_durations_df_normalized['Sync'], bar_width, color='tab:blue', label='SYNC', edgecolor='black', hatch=hatching[0])
ax2.bar(index + 2*bar_width, node_state_durations_df_normalized['Freq'], bar_width, color='tab:red', label='FREQ', edgecolor='black', hatch=hatching[2])

ax2.set_xlabel('Node ID')
ax2.set_xticks(index + 1.5 * bar_width)
ax2.set_xticklabels(node_state_durations_df_normalized.index)
ax2.set_ylabel('Duration (\\%)')
ax2.set_ylim(0.01, 100)
ax2.set_yscale('log')
ax2.grid(axis='y', linestyle='-', linewidth='0.5')
ax2.grid(axis='y', which='minor', linestyle=':', linewidth='0.5')
ax2.minorticks_on()
handles, labels = ax2.get_legend_handles_labels()
ax2.legend(handles, labels, title='NTP state', loc='upper center', fontsize='small', ncol=3, handleheight=1.5, bbox_to_anchor=(0.425, 1.3), handletextpad=0.5)
fig2.savefig(f'fig/{file}-node-state-durations.pdf', bbox_inches='tight', dpi=1200)


# Calculate delays between successive rows in aex_df
mem_aex_df["delay"] = mem_aex_df["datetime"].groupby(aex_df["ID"]).diff().fillna(0)

# Plot histogram of delays
fig3, ax3 = plt.subplots(figsize=(4.5, 1.5))
for group, color, linestyle in zip(mem_aex_df.groupby("ID"),colors, linestyles):
  ax3.hist(group[1]["delay"], bins=100, color=color, linestyle=linestyle, cumulative=True, histtype='step', density=True, label=f"Node {1+int(group[0][:-2])%12345}")
ax3.set_xlabel('Delay between successive AEXs (s)')
ax3.set_ylabel('Cumulative frequency')
ax3.grid(True)
ax3.grid(which='minor', linestyle=':', linewidth='0.5')
ax3.grid(which='major', linestyle='-', linewidth='0.5')
ax3.minorticks_on()
ax3.set_xlim(0)
ax3.set_ylim(0, 1)
ax3.set_yticks(np.arange(0, 1.01, 0.25))
ax3.set_yticks(np.arange(0, 1.01, 0.05), minor=True)
ax3.set_yticklabels(['{:.0f}\\%'.format(x * 100) for x in ax3.get_yticks()])

if LOW_INTERRUPTS:
  max_x_value = math.ceil(mem_aex_df["delay"].max())
  closest_multiple_of_60 = (max_x_value // 60 + 1) * 60

  ax3.set_xticks(np.arange(0, closest_multiple_of_60 + 1, 60))
  ax3.set_xlim(0, closest_multiple_of_60)
  ax3.set_xticks(np.arange(0, closest_multiple_of_60 + 1, 10), minor=True)
else:
  ax3.set_xticks(np.arange(0, 2.01, 0.2))
  ax3.set_xlim(0, 2)
  ax3.set_xticks(np.arange(0, 2.01, 0.05), minor=True)
  # ax3.set_xlim(0.01, 10)
  # ax3.set_xscale('log')

handles, labels = ax3.get_legend_handles_labels()
ax3.legend(handles[::-1], labels, loc='upper left', fontsize='small')
fig3.savefig(f'fig/{file}-aex-delays-histogram.pdf', bbox_inches='tight', dpi=1200)

# # Calculate delays between successive rows in merged dataframe's Node Time
# merged["node_time_delay"] = merged["datetime_node"].groupby(merged["ID_node"]).diff().fillna(0)

# # Plot histogram of Node Time delays
# POLL_PERIOD = 1 #[s]
# MIN_POWER_X = 5
# MIN_POWER_Y = 5

# fig4, ax4 = plt.subplots(figsize=(4.5, 1.5))
# merged=merged[merged["node_time_delay"] != 0]
# merged["node_time_delay"] = merged["node_time_delay"].apply(lambda x: x.total_seconds()-POLL_PERIOD)
# for group, color, linestyle in zip(merged.groupby("ID_node"), colors, linestyles):
#   hist_values, bin_edges = np.histogram(group[1]["node_time_delay"], bins=100, density=True)
#   cumulative_values = 1-np.cumsum(hist_values * np.diff(bin_edges))
#   ax4.step(bin_edges[:-1], cumulative_values, color=color, linestyle=linestyle, label=f"Node {1+int(group[0][:-2])%12345}")
# ax4.set_xlabel('Additional delay between successive TS polls (s)')
# ax4.set_ylabel('Cumulative frequency')
# ax4.grid(True)
# ax4.grid(which='minor', linestyle=':', linewidth='0.5')
# ax4.grid(which='major', linestyle='-', linewidth='0.5')
# ax4.set_yscale("log")
# ax4.set_xscale("log")
# ax4.set_xlim(1/10**MIN_POWER_X,1)
# ax4.set_ylim(1/10**MIN_POWER_Y,1)
# ax4.set_yticks([1/10**y for y in range(1, MIN_POWER_Y+1)][::-1])
# ax4.set_yticklabels(['{:.7f}'.format((1-x)) for x in ax4.get_yticks()])

# handles, labels = ax4.get_legend_handles_labels()
# ax4.legend(handles, labels, loc='upper right', fontsize='small')
# fig4.savefig(f'fig/{file}-node-time-delays-histogram.pdf', bbox_inches='tight', dpi=1200)

# # Calculate delays between successive rows in merged dataframe's Reference Time
# merged["ref_time_delay"] = merged["datetime_ref"].groupby(merged["ID_node"]).diff().fillna(0)
# merged["ref_time_delay"] = merged["ref_time_delay"].apply(lambda x: x-POLL_PERIOD)
# # Plot histogram of Reference Time delays
# fig5, ax5 = plt.subplots(figsize=(4.5, 1.5))
# for group, color, linestyle in zip(merged.groupby("ID_node"), colors, linestyles):
#   hist_values, bin_edges = np.histogram(group[1]["ref_time_delay"], bins=100, density=True)
#   cumulative_values = 1 - np.cumsum(hist_values * np.diff(bin_edges))
#   ax5.step(bin_edges[:-1], cumulative_values, color=color, linestyle=linestyle, label=f"Node {1+int(group[0][:-2])%12345}")
# ax5.set_xlabel('Delay between successive Reference Time polls (s)')
# ax5.set_ylabel('Cumulative frequency')
# ax5.grid(True)
# ax5.grid(which='minor', linestyle=':', linewidth='0.5')
# ax5.grid(which='major', linestyle='-', linewidth='0.5')
# ax5.set_yscale("log")
# ax5.set_xscale("log")
# ax5.set_xlim(1 / 10**MIN_POWER_X, 1)
# ax5.set_ylim(1/10**MIN_POWER_Y, 1)
# ax5.set_yticks([1/10**y for y in range(1, MIN_POWER_Y+1)][::-1])
# ax5.set_yticklabels(['{:.7f}'.format((1 - x)) for x in ax5.get_yticks()])

# handles, labels = ax5.get_legend_handles_labels()
# ax5.legend(handles, labels, loc='upper right', fontsize='small')
# fig5.savefig(f'fig/{file}-ref-time-delays-histogram.pdf', bbox_inches='tight', dpi=1200)