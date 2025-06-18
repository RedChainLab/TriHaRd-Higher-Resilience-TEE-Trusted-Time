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

if len(sys.argv) > 1:
  log = (sys.argv[1] == "1")

# Get all files in out/count directory that match sleep_type and sleep_time_secs
import os
import re
import progressbar
files = os.listdir('out/count-triad')
files = [f for f in files if re.match(f'counts_clean.*', f)]
#print(files)

# For each timestamp, plot the count distribution
for file in files:
    #print(files_timestamp)
    agg_df = pd.DataFrame()
    try:
      with open(f'out/count-triad/{file}', 'r') as f:
        agg_df = pd.read_csv(f, header=None)
    except Exception as e:
      print(f'Error reading {file}: {e}\n')
    agg_df.columns = ['count']
    fig, ax = plt.subplots()
    ax.hist(agg_df['count'], bins=200, label='AEX', histtype='step', linewidth=1)
    ax.hist(agg_df['count'], bins=200, label='AEX', histtype='step', linewidth=1, cumulative=True)
    ax.set_xlabel('Number of increments')
    ax.set_ylabel('Number of runs')

    y_closest_power_of_10 = 10 ** (math.floor(math.log10(agg_df['count'].count()))-1)
    ax.set_yticks(np.arange(0, agg_df['count'].count()+y_closest_power_of_10/2, y_closest_power_of_10))

    x_closest_power_of_10 = 10 ** math.floor(math.log10(agg_df['count'].max()))
    #ax.set_xticks(np.arange(0, agg_df['count'].max()+x_closest_power_of_10/2, x_closest_power_of_10/2))
    #ax.set_xticks(np.arange(0, agg_df['count'].max()+x_closest_power_of_10/2, x_closest_power_of_10/10), minor=True)

    #ax.set_xlim(agg_df['count'].min()-x_closest_power_of_10/2, agg_df['count'].max()+x_closest_power_of_10/2)
    if log:
      ax.set_yscale('log')
    ax.grid(True, which='major', linestyle='-')
    ax.grid(True, which='minor', linestyle=':', alpha=0.8)

    rel_error=(agg_df['count'].max()-agg_df['count'].min())/agg_df['count'].mean()
    ax.legend(labels=["$\\frac{\\Delta x}{\\bar{x}}="+f'{rel_error:2E}$'], loc='center right')

    print("mean:",agg_df['count'].mean(),"std:",agg_df['count'].std())

    fig.savefig(f'fig/{file}{"-log" if log else ""}.png', bbox_inches='tight', dpi=1200)

