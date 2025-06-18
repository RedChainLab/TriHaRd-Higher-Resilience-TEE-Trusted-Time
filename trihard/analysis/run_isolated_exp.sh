# Does not work together with sim_interrupts.sh yet (AEXs do not seem to be triggered)
NODE_PORT=$1
TSC_CORE=$2
TSC_FREQ=$3
SLEEP_ATTACK_MS=$4
APP_NAME=$5
LOG_PATH=$6
EXP_PREFIX=$7

STARTING_CORE=$8
ENDING_CORE=$9
N_NODES=${10}
EXPERIMENT_TIME=${11}

sleep_attack_ms=$SLEEP_ATTACK_MS

PEER_IP="127.0.0.1"

for i in $(seq 1 $N_NODES); do
    starting_core=$((STARTING_CORE + i - 1))
    tsc_core=$((TSC_CORE + i - 1))
    node_port=$((NODE_PORT + i - 1))
    peer_port_1=$((NODE_PORT + (i % N_NODES)))
    peer_port_2=$((NODE_PORT + ((i + 1) % N_NODES)))

    echo "Starting node $i on core $tsc_core with port $node_port"
    isolation_cmd="taskset -c ${tsc_core},${starting_core}-${ENDING_CORE}:${N_NODES}"
    echo "Running command: ${isolation_cmd} ./${APP_NAME} ${node_port} ${tsc_core} ${TSC_FREQ} ${sleep_attack_ms} 1 ${EXPERIMENT_TIME} ${PEER_IP} ${peer_port_1} ${PEER_IP} ${peer_port_2}"
    ${isolation_cmd} ./${APP_NAME} ${node_port} ${tsc_core} ${TSC_FREQ} ${sleep_attack_ms} 1 ${EXPERIMENT_TIME} ${PEER_IP} ${peer_port_1} ${PEER_IP} ${peer_port_2} > "${LOG_PATH}/triad-isol-${EXP_PREFIX}-${i}-`date +%Y-%m-%d-%H-%M-%S`.log" &
    sleep_attack_ms=0
done