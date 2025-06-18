for i in 1 2 3
do 
    scp -r trihard-node-$i:~/triad/out/log/triad-*.log ../out_deployed/log
done
for f in `ls ../out_deployed/log/triad-12345-*.log`
do
    prefix=`echo $f | cut -d '-' -f1 | rev | cut -d '/' -f1 | rev`
    suffix=`echo $f | cut -d '-' -f3- | rev | cut -d '-' -f2- | rev`
    cat ../out_deployed/log/$prefix-12345-$suffix-* > out/log/$prefix-$suffix.log
    cat ../out_deployed/log/$prefix-12346-$suffix-* >> out/log/$prefix-$suffix.log
    cat ../out_deployed/log/$prefix-12347-$suffix-* >> out/log/$prefix-$suffix.log
done