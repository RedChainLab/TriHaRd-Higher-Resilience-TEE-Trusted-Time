TEST_NAME := test_timespec.out
SRC_FILE := test_timespec.cpp

all: build

test: build
	@echo "Running tests..."
	@./${TEST_NAME}

build: ${TEST_NAME}

${TEST_NAME}: ${SRC_FILE}
	@echo "Building ${TEST_NAME}..."
	g++ -I../Enclave ${SRC_FILE} -o ${TEST_NAME}

clean:
	rm -f ${TEST_NAME}

.PHONY: all test build clean