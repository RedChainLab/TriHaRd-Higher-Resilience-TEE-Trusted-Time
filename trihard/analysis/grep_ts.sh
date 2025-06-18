
LOG_PATH=out/log
OUT_PATH=out/ts
mkdir -p $LOG_PATH $OUT_PATH
year=$(date +%y)
egrep -o "\[Node [0-9]*\]> (Node Time|Ref\. Time).* UTC" $LOG_PATH/$1.log > $OUT_PATH/$1-ts-node.log
egrep -o "\[utrst-Handler [0-9]*\]> TS Time: [0-9]{2}/[0-9]{2}/$year.* UTC" $LOG_PATH/$1.log > $OUT_PATH/$1-aex.log
egrep -o "\[utrst-ENode [0-9]*\]> TS Time: [0-9]{2}/[0-9]{2}/$year.* UTC" $LOG_PATH/$1.log > $OUT_PATH/$1-ut-node.log
egrep -o "\[utrst-TA [0-9]*\]> TS Time: [0-9]{2}/[0-9]{2}/$year.* UTC" $LOG_PATH/$1.log > $OUT_PATH/$1-ut-ta.log
egrep -o "\[utrst-StateSwitch [0-9]*\]> (Tainted|OK|FullCalib) Time: [0-9]{2}/[0-9]{2}/$year.* UTC" $LOG_PATH/$1.log > $OUT_PATH/$1-states.log
egrep -o "\[utrst-StateSwitch [0-9]*\]> (PeerConsistent|TAConsistent|TAInconsistent) Time: [0-9]{2}/[0-9]{2}/$year.* UTC" $LOG_PATH/$1.log > $OUT_PATH/$1-clock-states.log
egrep -o "\[utrst-StateSwitch [0-9]*\]> (Freq|Sync|Spike) Time: [0-9]{2}/[0-9]{2}/$year.* UTC" $LOG_PATH/$1.log > $OUT_PATH/$1-node-states.log