all: build

deps:
	@if [ ! -f ok.log ]; then \
		if ! test -d libsodium-1.0.18; then \
			if ! test -f libsodium-1.0.18.tar.gz; then \
				wget https://github.com/jedisct1/libsodium/releases/download/1.0.18-RELEASE/libsodium-1.0.18.tar.gz; \
			fi; \
			tar -xf libsodium-1.0.18.tar.gz; \
		fi; \
		cd libsodium-1.0.18 && ./configure && make && make check && sudo make install && touch ../ok.log; \
		cd .. && rm -rf libsodium-1.0.18 libsodium-1.0.18.tar.gz; \
	fi

build: deps
	@g++ -g -Wall -Werror -o server server.cpp -lsodium
	@echo "Built Time Authority. Run './server <roundtrip-netw-ms>' in time_authority directory to start the server."

clean:
	rm -f server

.PHONY: all deps build clean