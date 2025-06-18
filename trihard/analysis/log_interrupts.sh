mkdir -p out
if [ $# != 1 ]; then
    echo "Usage: $0 <core_id_to_interrupt>"
    exit 1
fi
echo "$1;`date +%Y-%m-%d-%H-%M-%S`" >> out/interrupts.log; analysis/sim_interrupts.sh $1 3400-0.01 3300-0.532 3300-1.5895; echo "$1;`date +%Y-%m-%d-%H-%M-%S`">> out/interrupts.log