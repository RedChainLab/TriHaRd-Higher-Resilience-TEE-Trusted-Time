FREQ=3000000000
start=$((0x`sudo rdmsr 0x10 -p $2`))
offset=`expr $4 \* $FREQ`
new_start=`expr $start $3 $offset`
if [ -z $5 ];
then
sudo wrmsr 0x10 $new_start -p $1
else
    while true;
    do
        sudo wrmsr 0x10 $new_start -p $1
        echo wrmsr
        sleep $4
    done
fi
end=$((0x`sudo rdmsr 0x10 -p $1`))
echo "\t\t\t0dTSC\t\t\t0xTSC\t\t\tDiff. to start"
echo "Read TSC start value:\t0d$start\t0x`printf '%x' $start`"
echo "Written TSC value:\t0d$new_start\t0x`printf '%x' $new_start`\t\t`expr $new_start - $start`"
echo "Read TSC end value:\t0d$end\t0x`printf '%x' $end`\t\t`expr $end - $start`"