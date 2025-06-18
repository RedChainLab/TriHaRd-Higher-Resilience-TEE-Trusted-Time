/*
 * Copyright (C) 2011-2021 Intel Corporation. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in
 *     the documentation and/or other materials provided with the
 *     distribution.
 *   * Neither the name of Intel Corporation nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


#ifndef _ENCLAVE_H_
#define _ENCLAVE_H_

#include <stdlib.h>
#include <assert.h>

#include "sodium.h"
#include <sgx_thread.h>
#include <vector>
#include <string>

#if defined(__cplusplus)
extern "C" {
#endif

int printf(const char *fmt, ...);

#if defined(__cplusplus)
}
#endif

#define SIZE 1024

enum {
    SUCCESS = 0,
    SOCKET_ALREADY_EXISTS = -1,
    SOCKET_CREATION_ERROR = -2,
    SOCKET_BINDING_ERROR = -3,
    READING_ERROR = -4,
    DECRYPTION_FAILED = -5,
    SENDING_ERROR = -6,
    SOCKET_INEXISTENT = -7
}; 

typedef enum node_state{
    SYNC_STATE=0,
    SPIK_STATE=1,
    FREQ_STATE=2
} node_state_t;

typedef enum clock_state {
    TA_CONSISTENT_STATE=1,
    TA_INCONSISTENT_STATE=2
} clock_state_t;

typedef enum callers{
    HANDLER=0,
    ENODE=1,
    TA=2,
    OK=3,
    TAINTED=4,
    REF_CALIB=5,
    FULL_CALIB=6,
    TA_INCONSISTENT=7,
    TA_CONSISTENT=8,
    FREQ=10,
    SPIK=11,
    SYNC=12,
    INFO=100
} callers_t;

const int PEER_REGISTER_SIZE = 8;
typedef struct PeerRegister {
    double offset[PEER_REGISTER_SIZE]; // theta
    double delay[PEER_REGISTER_SIZE];  // delta
    double dispersion[PEER_REGISTER_SIZE]; // phi
} PeerRegister_t;

typedef struct {
    int port;
    long long int* add_count;
    long long int* mem_add_count;

    long long int* total_aex_count;

    bool* tainted;
    sgx_thread_mutex_t* tainted_mutex;
    sgx_thread_cond_t* tainted_cond;

    long long int* aex_count;
    long long int* monitor_aex_count;

    long long int* count_aex;
    long long int* monitor_aex;

    bool* calib_ts_ref;
    bool* calib_count;

    long long int* curr_tsc;
    double* tsc_freq;
}aex_handler_args_t;

typedef struct {
    long long int msg_id;
    long long int total_aex_count;
    long long int tsc;
}calib_msg_t;

class Peer
{
public:
    std::string hostname;
    uint16_t port;
    
    std::pair<long long int, bool> last_ts_is_consistent;
    bool views_as_consistent;

    Peer(std::string _hostname, uint16_t _port):hostname(_hostname), port(_port),last_ts_is_consistent(std::make_pair(0, false)),
        views_as_consistent(false)
    {
    }
};

class ENode
{
public:
    int port;
    bool stop;

    long long int add_count;
    long long int total_aex_count;
    long long int nb_uninterr_rounds;
    const long long int MAX_UNINTERRUPTED_ROUNDS = 0;

    node_state_t node_state;
    clock_state_t clock_state;

    const double TA_CONSISTENT_THRESHOLD = 0.95;
    const double PEER_CONSISTENT_THRESHOLD = 0.95;

    //NTP constants
    const long long STEP_OUT_THRESHOLD_S = 100; // [s] default: 300s
    const long long STEP_THRESHOLD_NS = 128000000; // [ms] default: 128ms
    const long long CLOCK_FLOOR_NS = 500000; // [ns] aka CLOCK_FLOOR in NTPsec
    const double DISPERSION_S = 16; // [s]
    const double MAX_DRIFT_RATE = 0.000015; // [s/s]

    const int PEER_REGISTERS_SIZE = 8;
    const int MIN_POLL_EXPONENT = 6; // [log2s] default: 6
    const int MAX_POLL_EXPONENT = 10; // [log2s] default: 10
    const int ALLAN_INTERCEPT = 11; // [log2s] default: 11 aka CLOCK_ALLAN in NTPsec
    const double CLOCK_PLL = .5; // default: 16
    const double CLOCK_FLL = 0.25; // default: 0.25
    const int CLOCK_PGATE = 4; // default: 4 for poll_exponent's "jiggle counter"
    const int CLOCK_LIMIT = 30; // default: 30 absolute incr/decrease threshold for poll_exponent's "jiggle counter"
    const double JITTER_EMA_ALPHA = 0.1; // exponential moving average alpha for jitter

    //NTP variables
    timespec clock_offset;

    long long int poll_exponent;
    long long int freq_count;

    double alphas[3]={0.1, 0.01, 0.001};
    double ta_consistencies[3]={0, 0, 0};
    double ta_consistency_scores[3]={0, 0, 0};
    const double PEER_EMA_TAU=10; // [s] default: 0.1s

    bool calib_count;
    bool tainted;
    long long int mem_add_count;

    long long int aex_count;
    long long int monitor_aex_count;

    long long int count_aex[SIZE];
    long long int monitor_aex[SIZE];

    sgx_thread_rwlock_t stop_rwlock;
    sgx_thread_rwlock_t socket_rwlock;

    sgx_thread_mutex_t tainted_mutex;
    sgx_thread_mutex_t calib_mutex;
    sgx_thread_cond_t tainted_cond;
    sgx_thread_cond_t untainted_cond;

    aex_handler_args_t aex_args;

    void monitor();
    int loop_recvfrom();
    void refresh();
    void untaint_trigger();
    void stop_tasks();

    int add_sibling(std::string hostname, uint16_t port);

    timespec get_timestamp(bool force=false);

    void shadow_tsc_update();

    ENode(int _port, double _tsc_freq, double _shadow_tsc_freq, int _sleep_attack_ms=0);
    ~ENode();

private:
    int sock;

    std::pair<std::string, uint16_t> time_authority;

    int sleep_time;
    int sleep_attack_ms;
    int verbosity;

    long long tsc;
    double tsc_freq;
    double shadow_tsc_freq;
    bool attacker;
    
    long long tsc_offset;
    const int N_TSC_UPDATES=1;

    long long tsc_ref;
    long long last_update_tsc;
    timespec ts_ref;

    long long int calib_msg_count;
    static const int NB_CALIB_MSG = 1;
    calib_msg_t calib_sent[NB_CALIB_MSG];
    calib_msg_t calib_recvd[NB_CALIB_MSG];

    long long add_count_ref;

    unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES];
    unsigned char key[crypto_aead_aes256gcm_KEYBYTES];

    std::vector<Peer> siblings;

    bool monitor_stopped;
    bool refresh_stopped;
    bool trigger_stopped;
    bool readfrom_stopped;

    bool should_stop();

    void ntp();
    void step_clock();
    void discipline_clock();
    void select_clock();

    int setup_socket();

    bool calibrate();
    bool calibrate_drift();
    bool calibrate_count();
    bool monitor_rdtsc();

    int handle_message(const void* buff, size_t buff_len, char* ip, uint16_t port);
    int sendMessage(const void* buff, size_t buff_len, const char* ip, uint16_t port);

    void sendTaintedMessages();

    void send_recv_drift_message(int sleep_time_ms, int sleep_attack_ms=0);

    void incrementNonce();
    int encrypt(const unsigned char* plaintext, const unsigned long long plen, unsigned char* ciphertext, unsigned long long* clen);
    int decrypt(const unsigned char* ciphertext, const unsigned long long clen, unsigned char* decrypted, unsigned long long* dlen);
    int test_encdec();

    void eprintf(const char *fmt, ...);

    void print_siblings();
    void print_state(callers_t caller);
};

#endif /* !_ENCLAVE_H_ */
