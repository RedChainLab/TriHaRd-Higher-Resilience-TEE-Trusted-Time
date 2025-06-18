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

log=False

# Files in out/count directory are in the naming format count-<timestamp>-<sgx_type>-<sleep_type>-<sleep_time_secs>-<repeat_id>.csv
# Get <sleep_type> <sleep_time_secs> from command line arguments
sgx_type = sys.argv[1]
sleep_type = sys.argv[2]
sleep_time_secs = sys.argv[3]
if len(sys.argv) > 4:
  log = (sys.argv[4] == "1")

# Get all files in out/count directory that match sleep_type and sleep_time_secs
import os
import re
import progressbar
files = os.listdir('out/count1')
files = [f for f in files if re.match(f'count-.*-{sgx_type}-{sleep_type}-{sleep_time_secs}-.*', f)]
#print(files)

# Group files by timestamp
timestamps = set(["-".join(f.split('-')[1:7]) for f in files])
#print(timestamps)

# For each timestamp, plot the count distribution
for timestamp in timestamps:
    files_timestamp = [f for f in files if timestamp=="-".join(f.split('-')[1:7])]
    #print(files_timestamp)
    agg_df = pd.DataFrame()
    print(f'Processing {timestamp}...')
    for f in progressbar.progressbar(files_timestamp):
      try:
        with open(f'out/count1/{f}', 'r') as file:
            df = pd.read_csv(file, sep=';', header=0, index_col=0)
            #print(df)
            agg_df = pd.concat([agg_df, df.tail(1)])
      except Exception as e:
        print(f'Error reading {f}: {e}\n')
    agg_df.columns = ['count']
    fig, ax = plt.subplots(figsize=(4.5,2.5))
    # ax.hist(agg_df['count'], bins=200, label='AEX', histtype='step', linewidth=1)
    ax.hist(agg_df['count'], bins=200, label='AEX', histtype='step', linewidth=1, cumulative=True)
    ax.set_xlabel('Number of increments')
    ax.set_ylabel('Number of runs')

    y_closest_power_of_10 = 10 ** (math.floor(math.log10(agg_df['count'].count()))-1)
    if not log:
      ax.set_yticks(np.arange(0, agg_df['count'].count()+y_closest_power_of_10/2, y_closest_power_of_10))

    x_closest_power_of_10 = 10 ** math.floor(math.log10(agg_df['count'].max()))
    #ax.set_xticks(np.arange(0, agg_df['count'].max()+x_closest_power_of_10/2, x_closest_power_of_10/2))
    #ax.set_xticks(np.arange(0, agg_df['count'].max()+x_closest_power_of_10/2, x_closest_power_of_10/10), minor=True)

    #ax.set_xlim(agg_df['count'].min()-x_closest_power_of_10/2, agg_df['count'].max()+x_closest_power_of_10/2)
    ax.set_ylim(0)
    if log:
      ax.set_yscale('log')
      ax.set_ylim(0.5)
    ax.grid(True, which='major', linestyle='-')
    ax.grid(True, axis="y", which='minor', linestyle='-', alpha=0.5)
    ax.minorticks_on()

    # rel_error=(agg_df['count'].max()-agg_df['count'].min())/agg_df['count'].mean()
    # ax.legend(labels=["$\\frac{\\Delta x}{\\bar{x}}="+f'{rel_error:2E}$'], loc='center right')

    print("mean:",agg_df['count'].mean(),"std:",agg_df['count'].std())

    fig.savefig(f'fig/count-{timestamp}-{sgx_type}-{sleep_type}-{sleep_time_secs}{"-log" if log else ""}.pdf', bbox_inches='tight', dpi=1200)

