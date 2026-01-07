APP_NAME=trihard
SO_PATH=node/libtrihard.so
TIME_AUTH_DIR=time_authority
TIME_AUTH_PATH=$(TIME_AUTH_DIR)/server

LOG_PATH=out/log
OUT_PATH=out/ts

SLEEP_ATTACK_MS?=0
FREQ_OFFSET?=0

NODE_PORT?=12345
TSC_CORE?= 2
TSC_FREQ?= $(shell sudo dmesg | grep "MHz processor" | rev | cut -d" " -f 3 | rev)
EXP_PREFIX="default"

USE_AEX_NOTIFY ?= $(shell ../SGX-hardware/test-sgx | grep "EDECCSSA" | rev | cut -c 1)
USE_SGX2 := $(shell ../SGX-hardware/test-sgx | grep "SGX2 leaf" | rev | cut -c 1)

SINGLE_HOST?=1

THROUGHPUT?=0
LATENCY?=0

STARTING_CORE?=8
ENDING_CORE?=13
N_NODES?=3
EXPERIMENT_TIME?=5

Cpp_Flags=-Wall -Werror -Wextra
ifeq ($(SINGLE_HOST), 1)
	Cpp_Flags += -DSINGLE_HOST
endif
ifeq ($(THROUGHPUT), 1)
	Cpp_Flags += -DTHROUGHPUT
endif
ifeq ($(LATENCY), 1)
	Cpp_Flags += -DLATENCY
endif

all: build

deps:
	sudo apt update
	sudo apt-get install -y libcap-dev # for SGX-hardware
	sudo apt-get install -y python3 python3-pip
	sudo apt-get install -y dvipng texlive-latex-extra texlive-fonts-recommended cm-super
	pip3 install -r requirements.txt
	make -C $(TIME_AUTH_DIR) deps

build: $(SO_PATH) $(APP_NAME) $(TIME_AUTH_PATH)

run: build
	./$(APP_NAME) $(NODE_PORT) $(TSC_CORE) $(TSC_FREQ) $(FREQ_OFFSET) $(SLEEP_ATTACK_MS) $(N_NODES) $(EXPERIMENT_TIME)

exp: build
	mkdir -p ${LOG_PATH}
	@echo "${LOG_PATH}/trihard-$(USE_SGX2)SGX2-$(USE_AEX_NOTIFY)AEXNotify-$(EXPERIMENT_TIME)min-$(FREQ_OFFSET)off-$(SLEEP_ATTACK_MS)ms-$(EXP_PREFIX)-`date +%Y-%m-%d-%H-%M-%S`.log"
	./$(APP_NAME) $(NODE_PORT) $(TSC_CORE) $(TSC_FREQ) $(FREQ_OFFSET) $(SLEEP_ATTACK_MS) $(N_NODES) $(EXPERIMENT_TIME) > "${LOG_PATH}/trihard-$(USE_SGX2)SGX2-$(USE_AEX_NOTIFY)AEXNotify-$(EXPERIMENT_TIME)min-$(FREQ_OFFSET)off-$(SLEEP_ATTACK_MS)ms-$(EXP_PREFIX)-`date +%Y-%m-%d-%H-%M-%S`.log"

isol-exp: build
	mkdir -p ${LOG_PATH}
	./analysis/run_isolated_exp.sh $(NODE_PORT) $(TSC_CORE) $(TSC_FREQ) $(FREQ_OFFSET) $(SLEEP_ATTACK_MS) $(APP_NAME) $(LOG_PATH) $(EXP_PREFIX) $(STARTING_CORE) $(ENDING_CORE) $(N_NODES) $(EXPERIMENT_TIME)

test:
	@echo "Running tests..."
	@cd node/test && make test

$(SO_PATH):
	@cd node && make USE_AEX_NOTIFY=$(USE_AEX_NOTIFY) USE_SGX2=$(USE_SGX2) SINGLE_HOST=$(SINGLE_HOST)

$(APP_NAME):
	@g++ -o $(APP_NAME) $(Cpp_Flags) -Wl,-rpath,node -Inode -I/opt/intel/sgxsdk/include user/main.cpp -Lnode -ltrihard

$(TIME_AUTH_PATH):
	@cd $(TIME_AUTH_DIR) && make build

clean:
	rm -f $(APP_NAME)

cleanall: clean
	cd node && make clean
	cd $(TIME_AUTH_DIR) && make clean
	rm -f $(SO_PATH)
	cd node/test && make clean

.PHONY: all build run clean deps test exp isol-exp cleanall