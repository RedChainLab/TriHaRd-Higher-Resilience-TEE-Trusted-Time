latest=`ls -c out/log | head -n 1 | rev | cut -d. -f2- | rev`
analysis/watch_run.sh $1 $latest