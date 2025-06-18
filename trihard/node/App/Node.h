#ifndef NODE_H
#define NODE_H

#include <iostream>
#include <map>
#include <thread>
#include <vector>

#include <sgx_urts.h>
#include <sgx_uswitchless.h>

class Node {
public:
    static Node* get_instance(uint16_t port, int core_rdTSC, double tsc_freq, double shadow_tsc_freq, int sleep_attack_ms=0);
    static void destroy_instance(uint16_t port);
    timespec get_timestamp();
    void print_timestamp();
    void poll_timestamp(int count, int sleep_time_us=1000000, bool threaded=true);
    void bench_timestamp();
    void bench_lat_timestamp();
    int add_sibling(const std::string& hostname, uint16_t port);
    bool stop;
private:
    uint16_t port;
    int sleep_attack_ms;
    int core_rdTSC;
    double tsc_freq;
    double shadow_tsc_freq;
    std::map<std::pair<std::string, uint16_t>, int> siblings;
    std::vector<std::thread> threads;

    Node(uint16_t _port, int _core_rdTSC, double _tsc_freq, double _shadow_tsc_freq, int _sleep_attack_ms=0);
    ~Node();
    int initialize_enclave(const sgx_uswitchless_config_t* us_config);
    static std::map<int, Node*> nodes;
    sgx_enclave_id_t enclave_id;
    static const char* ENCLAVE_FILE;
    std::string getPrefix();
};

#endif // NODE_H