#include <unistd.h>

#include "App/Node.h"

#define POLL_PERIOD 1E5 // 1E6µs

int main(int argc, char* argv[]) {
    if (argc < 8 || argc % 2 != 0) {
        std::cerr << "Usage: " << argv[0] << " <port> <core_rdTSC> <tsc_freq> <freq_offset> <sleep_attack_ms> <n-nodes> <experiment-time> [<hostname> <port> ...]" << std::endl;
        return -1;
    }

    uint16_t port = (uint16_t)atoi(argv[1]);
    int core_rdTSC = atoi(argv[2]);
    double tsc_freq = atof(argv[3])/1000; // Convert from MHz to GHz
    if (tsc_freq <= 0) {
        std::cerr << "Error: tsc_freq must be greater than 0" << std::endl;
        return -1;
    }
    std::cout<<tsc_freq<<std::endl;
    double freq_offset = atof(argv[4])/1000; // Convert from MHz to GHz
    int sleep_attack_ms = atoi(argv[5]);
    int n_nodes = atoi(argv[6]);
    double exp_time = atof(argv[7]);
    if (exp_time <= 0) {
        std::cerr << "Error: experiment-time must be greater than 0" << std::endl;
        return -1;
    }
    if (n_nodes < 1) {
        std::cerr << "Error: n_nodes must be greater than 0" << std::endl;
        return -1;
    }

    Node* node = Node::get_instance(port, core_rdTSC, tsc_freq+freq_offset, tsc_freq, sleep_attack_ms);
#ifdef SINGLE_HOST
    Node* node2 = nullptr;
    Node* node3 = nullptr;
    Node* node4 = nullptr;
    if(n_nodes>1) node2 = Node::get_instance(port+1, core_rdTSC+1, tsc_freq, tsc_freq);
    if(n_nodes>2) node3 = Node::get_instance(port+2, core_rdTSC+2, tsc_freq, tsc_freq);
    if(n_nodes>3) node4 = Node::get_instance(port+3, core_rdTSC+3, tsc_freq, tsc_freq);
#endif

    usleep(10000);

#ifdef SINGLE_HOST
    if(n_nodes>1) node->add_sibling("127.0.0.1", port+1);
    if(n_nodes>2) node->add_sibling("127.0.0.1", port+2);
    if(n_nodes>3) node->add_sibling("127.0.0.1", port+3);
    if(n_nodes>2) node2->add_sibling("127.0.0.1", port+2);
    if(n_nodes>3) node2->add_sibling("127.0.0.1", port+3);
    if(n_nodes>3) node3->add_sibling("127.0.0.1", port+3);
#endif
    for(int i=8; i<argc; i+=2)
    {
        std::string hostname = argv[i];
        uint16_t port = atoi(argv[i+1]);
        node->add_sibling(hostname, port);
    }

    usleep(1000000);

#ifdef THROUGHPUT
    node->bench_timestamp();
#ifdef SINGLE_HOST
    if(n_nodes>1) node2->bench_timestamp();
    if(n_nodes>2) node3->bench_timestamp();
    if(n_nodes>3) node4->bench_timestamp();
#endif // SINGLE_HOST
#elif defined(LATENCY)
    node->bench_lat_timestamp();
#ifdef SINGLE_HOST
    if(n_nodes>1) node2->bench_lat_timestamp();
    if(n_nodes>2) node3->bench_lat_timestamp();
    if(n_nodes>3) node4->bench_lat_timestamp();
#endif // SINGLE_HOST
#else
    node->encl_poll_timestamp(-1, POLL_PERIOD);
#ifdef SINGLE_HOST
    if(n_nodes>1) node2->encl_poll_timestamp(-1, POLL_PERIOD);
    if(n_nodes>2) node3->encl_poll_timestamp(-1, POLL_PERIOD);
    if(n_nodes>3) node4->encl_poll_timestamp(-1, POLL_PERIOD);
#endif // SINGLE_HOST
#endif

    // std::string msg;
    // std::cout << "<Enter anything to stop>"<< std::endl;
    // std::cin >> msg;

    sleep((int)(exp_time*60));

    Node::destroy_instance(port);
#ifdef SINGLE_HOST
if(n_nodes>1) Node::destroy_instance(port+1);
if(n_nodes>2) Node::destroy_instance(port+2);
if(n_nodes>3) Node::destroy_instance(port+3);
#endif
    return 0;
}