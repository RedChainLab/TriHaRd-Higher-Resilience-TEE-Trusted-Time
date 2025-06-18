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
#include "sys/socket.h"
#include "Enclave_t.h"
#include "ENode.h"
#include <stdio.h>
#include <sgx_trts_aex.h>
#include <sgx_thread.h>
#include <map>
#include <math.h>
#include "timespec.h"
#include "ta.h"

#ifdef __cplusplus
extern "C" {
#endif

int printf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, BUFSIZ, fmt, args);
    va_end(args);
    ocall_print_string(buf);
    return 0;
}

#ifdef __cplusplus
}
#endif

#define S 1000000000
#define MS 1000000
#define US 1000

#define TAINTED_STR "Tainted"
#define UNTAINTING_STR "Untaint"
#define DRIFT_STR "Drift"

#define T_MSG_EXPECTED_LEN 79
#define U_MSG_EXPECTED_LEN 80
#define D_MSG_EXPECTED_LEN 81

std::map<int /*port*/, ENode*> nodes;

bool operator==(const Peer& lhs, const Peer& rhs)
{
    return (lhs.hostname == rhs.hostname && lhs.port == rhs.port);
}

static void printArray(long long int *arr, long long int size, long long int reference){
    /*
    Print a array of size SIZE, which contains the number of ADD operations performed before each AEX occurs.
    */
    for(int i = 0; i < size ; i++){
        printf("%d;%lld\n", i, arr[i]-reference);
    }
}

inline void log_aex(long long int* arr, long long int& next_index, long long int* add_count){
    if(next_index < SIZE)
    {
        arr[next_index++] = *add_count;
    }
    else
    {
        printf("Error: Array is full\n");
    }
}

inline double exponential_moving_average(double new_value, double old_value, double alpha)
{
    return (1-alpha)*old_value + alpha*new_value;
}

#ifdef USE_SGX2
inline long long int rdtscp(void){
    /*
    Read the TSC register
    */
    unsigned int lo, hi;
    __asm__ __volatile__("rdtscp" : "=a" (lo), "=d" (hi));
    //t_print("lo: %d, hi: %d\n", lo, hi);
    return ((uint64_t)hi << 32) | lo;
}
#else
inline long long int rdtscp(void){
    /*
    Read the TSC register using ocall
    */
    long long int tsc;
    ocall_readTSC(&tsc);
    return tsc;
}
#endif

#ifdef USE_AEX_NOTIFY
static void aex_handler(const sgx_exception_info_t *info, const void * args)
{
    /*
    a custom handler that will be called when an AEX occurs, storing the number of ADD operations (performed in another thread) in a global array. This allows you to 
    know when AEX occurs (the number of ADD operations increases linearly) and how often it occurs.
    */
    // long long int start_tsc=rdtscp();
    (void)info;
    const aex_handler_args_t* aex_args=(const aex_handler_args_t*)args;
    *aex_args->tainted = true;
    *aex_args->mem_add_count = *aex_args->add_count;
    *aex_args->total_aex_count += 1;
    sgx_thread_mutex_lock(aex_args->tainted_mutex);
    //printf("[Handler %d]> AEX %lld\r\n", aex_args->port, *aex_args->total_aex_count);
    timespec ts_ref;
    ocall_timespec_get(&ts_ref);
    ocall_timespec_print(&ts_ref, aex_args->port, HANDLER);
    if(*aex_args->calib_count)
    {
        ocall_timespec_print(&ts_ref, aex_args->port, TAINTED);
    }

    long long int mem_tsc=*(aex_args->curr_tsc);
    long long int postaex_tsc=rdtscp();
    *aex_args->curr_tsc = postaex_tsc;

    long long int total_nsec = (long long)((double)(postaex_tsc-mem_tsc)/(*aex_args->tsc_freq));
    // printf("[Handler %d]> AEX TSC: PREAEX_TSC=%lld, POSTAEX_TSC=%lld, DELTA=%lldns, TSCfreq=%.6f\r\n", aex_args->port, mem_tsc, postaex_tsc, total_nsec, *aex_args->tsc_freq);
    if(total_nsec<0)
    {
        printf("[Handler %d]> Error: Negative TSC difference detected!\r\n", aex_args->port);
    }
    else if(total_nsec>100000) // 100µs
    {
        printf("[Handler %d]> Error: Large TSC difference detected: %lldns!\r\n", aex_args->port, total_nsec);
    }
    
    // printf("[Handler %d]> Reference time: %.9fs\r\n", aex_args->port,  to_double(ts_ref));
    sgx_thread_cond_signal(aex_args->tainted_cond);
    sgx_thread_mutex_unlock(aex_args->tainted_mutex);
    // long long int end_tsc=rdtscp();
    // printf("[Handler %d]> AEX handler time: %lld cycles\r\n", aex_args->port, end_tsc-start_tsc);
    //log_aex(aex_args->count_aex, *(aex_args->aex_count), aex_args->add_count);
}
#endif

void ENode::untaint_trigger()
{
    while(!should_stop())
    {
        sgx_thread_mutex_lock(&tainted_mutex);
        if(tainted)
        {
            sgx_thread_cond_signal(&tainted_cond);
        }
        sgx_thread_mutex_unlock(&tainted_mutex);
        ocall_sleep(1);
    }
    trigger_stopped = true;
    eprintf("Untainting trigger stopped.\r\n");
}

void ENode::sendTaintedMessages()
{
    char send_buff[1024] = {0};
    long unsigned int msg_cursor=0;
    static long long int tainted_msg_count=0;
    memcpy(send_buff+msg_cursor, TAINTED_STR, strlen(TAINTED_STR));
    msg_cursor+=strlen(TAINTED_STR);
    memcpy(send_buff+msg_cursor, &tainted_msg_count, sizeof(tainted_msg_count));
    msg_cursor+=sizeof(tainted_msg_count);
    timespec org_ts=get_timestamp(true);
    memcpy(send_buff+msg_cursor, &org_ts, sizeof(org_ts));
    msg_cursor+=sizeof(org_ts);
    timespec dummy_ts;
    memcpy(send_buff+msg_cursor, &dummy_ts, sizeof(dummy_ts)); // rec_ts
    msg_cursor+=sizeof(dummy_ts);
    memcpy(send_buff+msg_cursor, &dummy_ts, sizeof(dummy_ts)); // xmt_ts
    msg_cursor+=sizeof(dummy_ts);
    memcpy(send_buff+msg_cursor, &dummy_ts, sizeof(dummy_ts)); // recv_ts
    msg_cursor+=sizeof(dummy_ts);
    for(unsigned int i=0; i<siblings.size(); i++)
    {
        if(verbosity>=1) eprintf("Sending untaint to %s:%d\r\n", siblings[i].hostname.c_str(), siblings[i].port);
        sendMessage(send_buff, msg_cursor, siblings[i].hostname.c_str(), siblings[i].port);
    }
    tainted_msg_count++;
}

void ENode::refresh()
{
    while(!should_stop())
    {
        sgx_thread_mutex_lock(&tainted_mutex);
        sgx_thread_cond_wait(&tainted_cond, &tainted_mutex);
        if(should_stop())
        {
            eprintf("Stopping refresh.\r\n");
            sgx_thread_mutex_unlock(&tainted_mutex);
            break;
        }
        else if(tainted && calib_count)
        {
            sgx_thread_mutex_unlock(&tainted_mutex);
            if(verbosity>=1) eprintf("Untainting\r\n");
            long long int mem_total_aex_count;
            do
            {
                shadow_tsc_update();
                mem_total_aex_count=total_aex_count;

                sendTaintedMessages();
                if(verbosity>=1) eprintf("Updating tsc for 10ms...\r\n");
                long long int reference_tsc=tsc;
                const int wait_time=10; //[ms]
                while(tainted && (tsc-reference_tsc<((long long int)(tsc_freq*MS))*wait_time) && !should_stop());
            } while (mem_total_aex_count!=total_aex_count && !should_stop());
            if(tainted)
            {
                if(verbosity>=1) eprintf("Peer untainting failed.\r\n");
                //print_state(REF_CALIB);
            }
        }
        else
        {
            eprintf("State (tainted, count)=(%d, %d) not ready.\r\n", tainted, calib_count);
            sgx_thread_mutex_unlock(&tainted_mutex);
        }
    }
    refresh_stopped = true;
    eprintf("Refresh stopped.\r\n");
}

ENode::ENode(int _port, double _tsc_freq, double _shadow_tsc_freq, int _sleep_attack_ms):port(_port), stop(false),
    add_count(0), total_aex_count(0), node_state(node_state::FREQ_STATE), clock_state(clock_state::TA_INCONSISTENT_STATE), clock_offset({0,0}), poll_exponent(2), calib_count(false),
    tainted(false), aex_count(0), monitor_aex_count(0), 
    sock(-1), time_authority(std::make_pair(TA_IP,TA_PORT)),sleep_time(500), sleep_attack_ms(_sleep_attack_ms), verbosity(0), tsc_freq(_tsc_freq), shadow_tsc_freq(_shadow_tsc_freq), tsc_offset(0),
    monitor_stopped(false), refresh_stopped(false), trigger_stopped(false)
{
    eprintf("Creating ENode instance...\r\n");
    memset(count_aex, 0, sizeof(count_aex));
    memset(monitor_aex, 0, sizeof(monitor_aex));

    attacker = (shadow_tsc_freq < tsc_freq);

    aex_args.port = port;

    aex_args.add_count = &add_count;
    aex_args.mem_add_count = &mem_add_count;

    aex_args.total_aex_count = &total_aex_count;

    aex_args.tainted = &tainted;
    aex_args.tainted_mutex = &tainted_mutex;
    aex_args.tainted_cond = &tainted_cond;

    aex_args.aex_count = &aex_count;
    aex_args.monitor_aex_count = &monitor_aex_count;
    aex_args.count_aex = count_aex;
    aex_args.monitor_aex = monitor_aex;
    
    aex_args.calib_count = &calib_count;

    aex_args.curr_tsc = &tsc;
    aex_args.tsc_freq = &tsc_freq;

    incrementNonce();
    //randombytes_buf(key, sizeof(key));
    const char* test_key = "b52c505a37d78eda5dd34f20c22540ea1b58963cf8e5bf8ffa85f9f2492505b4";
    sodium_hex2bin(key, crypto_aead_aes256gcm_KEYBYTES,
                       test_key, strlen(test_key),
                       NULL, NULL, NULL);

    sgx_thread_mutex_init(&tainted_mutex, NULL);
    sgx_thread_rwlock_init(&stop_rwlock, NULL);
    sgx_thread_rwlock_init(&socket_rwlock, NULL);
    sgx_thread_mutex_init(&calib_mutex, NULL);
    setup_socket();
    test_encdec();
    eprintf("ENode instance created.\r\n");
    print_state(callers::TA_INCONSISTENT);
    print_state(callers::FREQ);
}

void ENode::stop_tasks()
{
    eprintf("Stopping ENode instance...\r\n");
    print_siblings();
    eprintf("Sending stop...\r\n");
    sgx_thread_rwlock_wrlock(&stop_rwlock);
    stop=true;
    sgx_thread_rwlock_unlock(&stop_rwlock);
    while(!monitor_stopped&&!readfrom_stopped);
    eprintf("Monitor and readfrom tasks stopped.\r\n");
    eprintf("Signalling refresh to stop.\r\n");
    sgx_thread_mutex_lock(&tainted_mutex);
    sgx_thread_cond_signal(&tainted_cond);
    sgx_thread_mutex_unlock(&tainted_mutex);
    while(!refresh_stopped && !trigger_stopped);
    eprintf("Refresh and untaint trigger stopped.\r\n");
    eprintf("Closing socket...\r\n");
    sgx_thread_rwlock_wrlock(&socket_rwlock);
    close(sock);
    sock = -1;
    sgx_thread_rwlock_unlock(&socket_rwlock);
    eprintf("ENode tasks stopped.\r\n");
}

ENode::~ENode()
{
    sgx_thread_mutex_destroy(&tainted_mutex);
    sgx_thread_cond_destroy(&tainted_cond);
    sgx_thread_cond_destroy(&untainted_cond);
    sgx_thread_rwlock_destroy(&stop_rwlock);
    sgx_thread_rwlock_destroy(&socket_rwlock);
    sgx_thread_mutex_destroy(&calib_mutex);
}

void ENode::eprintf(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    std::string str = std::string("[ENode ") + std::to_string(port) + "]> ";
    str += fmt;
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, BUFSIZ, str.c_str(), args);
    va_end(args);
    ocall_print_string(buf);
}

bool ENode::should_stop()
{
    sgx_thread_rwlock_rdlock(&stop_rwlock);
    bool retval = stop;
    sgx_thread_rwlock_unlock(&stop_rwlock);
    return retval;
}

void ENode::incrementNonce(void)
{
    static bool init = false;
    if(!init)
    {
        memset(nonce, 0, sizeof(nonce));
        init = true;
    }
    else
    {
        /*for(unsigned int i = 0; i < crypto_aead_aes256gcm_NPUBBYTES; i++)
        {
            nonce[i]++;
            if(nonce[i] != 0)
            {
                break;
            }
        }*/
    }
}

int ENode::encrypt(const unsigned char* plaintext, const unsigned long long plen, unsigned char* ciphertext, unsigned long long* clen)
{
    incrementNonce();

    crypto_aead_aes256gcm_encrypt(ciphertext, clen,
                                  plaintext, plen,
                                  NULL, 0, NULL, nonce, key);
    
    return SUCCESS;
}

int ENode::decrypt(const unsigned char* ciphertext, const unsigned long long clen, unsigned char* decrypted, unsigned long long* dlen)
{
    if (crypto_aead_aes256gcm_decrypt(decrypted, dlen,
                                      NULL, ciphertext, clen,
                                      NULL, 0, nonce, key) != 0) {
        eprintf("Decryption failed\r\n");
        return -1;
    }
    return 0;
}

int ENode::test_encdec()
{
    unsigned char msg[] = "Hello!";
    unsigned long long msg_len = sizeof(msg);
    unsigned char ciphertext[sizeof(msg) + crypto_aead_aes256gcm_ABYTES];
    unsigned long long ciphertext_len = sizeof(ciphertext);
    unsigned char decrypted[sizeof(msg)];
    unsigned long long decrypted_len = sizeof(decrypted);
    eprintf("Message: %s\r\n", msg);
    encrypt(msg, msg_len, ciphertext, &ciphertext_len);
    eprintf("Encrypted: %s\r\n", ciphertext);
    if(decrypt(ciphertext, ciphertext_len, decrypted, &decrypted_len))
    {
        return DECRYPTION_FAILED;
    }
    eprintf("Decrypted: %s\r\n", decrypted);
    return SUCCESS;
}

int ENode::setup_socket()
{
    sgx_thread_rwlock_wrlock(&socket_rwlock);
    //creating a new server socket
    if ((this->sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        eprintf("server socket creation error...\r\n");
        return SOCKET_CREATION_ERROR;
    }

    //binding the port to ip and port
    struct sockaddr_in serAddr;
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons((uint16_t)port);
    serAddr.sin_addr.s_addr = INADDR_ANY;

    if ((bind(this->sock, (struct sockaddr*)&serAddr, sizeof(serAddr))) < 0) {
        eprintf("server socket binding error...: %d\r\n", errno);
        close(this->sock);
        return SOCKET_BINDING_ERROR;
    }

    struct timeval read_timeout{0,100000}; // 100ms
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof(read_timeout));
    sgx_thread_rwlock_unlock(&socket_rwlock);
    eprintf("server socket created...: %d\r\n", this->sock);
    return SUCCESS;
}

int ENode::loop_recvfrom()
{
    int retval=SUCCESS;
    while (retval == SUCCESS && !should_stop())
    {    
        char buff[1024] = {0};
        char ip[INET_ADDRSTRLEN];
        int cport;
        //eprintf("encl_recvfrom: %d, %p, %d, %d, %p, %p\r\n", sock, buff, sizeof(buff), 0, (struct sockaddr*)&cliAddr, &cliAddrLen);
        sgx_thread_rwlock_rdlock(&socket_rwlock);
        if(sock < 0)
        {
            sgx_thread_rwlock_unlock(&socket_rwlock);
            return SOCKET_INEXISTENT;
        }
        ssize_t readStatus = recvfrom(sock, buff, sizeof(buff), 0, ip, INET_ADDRSTRLEN, &cport);
        sgx_thread_rwlock_unlock(&socket_rwlock);
        if (readStatus < 0 && errno) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                //eprintf("waiting for data...: %d\r\n", errno);
                continue;
            } else {
                eprintf("reading error...: %d\r\n", errno);
                close(sock);
                sock = -1;
                return READING_ERROR;
            }
        }
        else if(readStatus > 0)
        {
            //eprintf("encl_recvfrom: %d, %p, %d, %d, %s, %d\r\n", sock, buff, sizeof(buff), 0, ip, cport);
            if(verbosity>=2) eprintf("loop_recvfrom: Message received from %s:%d if len %d: %s\r\n", ip, cport, readStatus, buff);

            unsigned char buff_dec[sizeof(buff)];
            unsigned long long buff_len_dec = sizeof(buff);
            retval=decrypt((unsigned char*)buff, readStatus, buff_dec, &buff_len_dec);
            if(retval)
            {
                eprintf("loop_recvfrom: Decryption failed.\r\n");
            }
            else
            {
                if(verbosity>=2) eprintf("loop_recvfrom: Decrypted message: %s\r\n", buff_dec);
            }

            retval=handle_message(buff_dec, buff_len_dec, ip, (uint16_t)cport);
        }
    }
    readfrom_stopped = true;
    eprintf("ENode listen stopped.\r\n");
    return retval;
}

int ENode::sendMessage(const void* buff, size_t buff_len, const char* ip, uint16_t cport)
{
    sgx_thread_rwlock_rdlock(&socket_rwlock);
    if(sock < 0)
    {
        sgx_thread_rwlock_unlock(&socket_rwlock);
        return SOCKET_INEXISTENT;
    }
    unsigned char buff_enc[buff_len + crypto_aead_aes256gcm_ABYTES];
    unsigned long long buff_len_enc = buff_len + crypto_aead_aes256gcm_ABYTES;
    int retval=encrypt((const unsigned char*)buff, buff_len, buff_enc, &buff_len_enc);
    if(verbosity>=2) eprintf("sendMessage: Encrypting message to %s:%d of len %d: %s\r\n", ip, cport, buff_len, buff);
    if(retval)
    {
        eprintf("sendMessage: Encryption failed.\r\n");
    }
    else
    {
        if(verbosity>=2) eprintf("sendMessage: Encrypted message of len %d: %s\r\n", buff_len_enc, buff_enc);
    }
    ssize_t sendStatus = -1;
    do {
        sendStatus = sendto(sock, buff_enc, buff_len_enc, 0, ip, INET_ADDRSTRLEN, cport);
        sgx_thread_rwlock_unlock(&socket_rwlock);
        if (sendStatus < 0 && errno) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                long long int reference_tsc=tsc;
                while(tsc-reference_tsc<((long long int)(tsc_freq*US))*10 && !should_stop());
            } 
            else 
            {
                eprintf("sending error on socket %d...: %d\r\n", sock, errno);
                sgx_thread_rwlock_wrlock(&socket_rwlock);
                close(sock);
                sock = -1;
                sgx_thread_rwlock_unlock(&socket_rwlock);
                return SENDING_ERROR;
            }
        }
    } while (sendStatus < 0 && (errno == EWOULDBLOCK  || errno == EAGAIN) && !should_stop());
    return SUCCESS;
}

int ENode::handle_message(const void* buff, size_t buff_len, char* ip, uint16_t cport)
{
    int retval = SUCCESS;
    if(buff_len == 0)
    {
        eprintf("Empty message received from %s:%d\r\n", ip, cport);
        return retval;
    }
    switch(((const char*)buff)[0])
    {
        case 'S':
        {
            if(verbosity>=2) eprintf("Sibling message received from %s:%d\r\n", ip, cport);
            if(std::find(siblings.begin(), siblings.end(), Peer(std::string(ip), cport)) == siblings.end())
            {
                siblings.emplace_back(ip, cport);
                if(verbosity>=2) eprintf("Sibling added.\r\n");
            }
            else
            {
                if(verbosity>=2) eprintf("Sibling already added.\r\n");
            }
            break;
        }
        case 'T':
        {
            if(verbosity>=1) eprintf("Tainted message received from %s:%d (len %d)\r\n", ip, cport, buff_len);
            auto sibling=std::find(siblings.begin(), siblings.end(), Peer(std::string(ip), cport));
            if(sibling == siblings.end())
            {
                eprintf("Tainted message received from unknown peer %s:%d\r\n", ip, cport);
                break;
            }
            else if(buff_len!=T_MSG_EXPECTED_LEN)
            {
                eprintf("Tainted message received from %s:%d with unexpected length %d\r\n", ip, cport, buff_len);
                break;
            }
            else if(clock_state==clock_state::TA_CONSISTENT_STATE)
            {
                if(sibling->port==12346)
                {
                    shadow_tsc_update();
                }
                timespec rec_ts=get_timestamp(true);

                if(verbosity>=1) eprintf("Received tainted msg at ts: %.9fs\r\n",to_double(rec_ts));

                long unsigned int offset_recv = strlen(TAINTED_STR);
                const long long int recvd_tainted_msg_count = *(const long long int*)((const char*)buff + offset_recv);
                offset_recv += sizeof(recvd_tainted_msg_count);
                const timespec org_ts = *(const timespec*)((const char*)buff + offset_recv);
                
                if(verbosity>=1)
                {
                    eprintf("Tainted Node Time:\t%.9fs\r\n", to_double(org_ts));
                    eprintf("Local Node Time:\t\t%.9fs\r\n", to_double(rec_ts));
                    eprintf("Peer is viewed as consistent: %d\r\n", sibling->last_ts_is_consistent.second);  
                }

                char send_buff[1024] = {0};
                long unsigned int msg_cursor=0;
                memcpy(send_buff+msg_cursor, UNTAINTING_STR, strlen(UNTAINTING_STR));
                msg_cursor+=strlen(UNTAINTING_STR);
                memcpy(send_buff+msg_cursor, &recvd_tainted_msg_count, sizeof(recvd_tainted_msg_count));
                msg_cursor+=sizeof(recvd_tainted_msg_count);
                memcpy(send_buff+msg_cursor, &sibling->last_ts_is_consistent.second, sizeof(sibling->last_ts_is_consistent.second));
                msg_cursor+=sizeof(sibling->last_ts_is_consistent.second);
                memcpy(send_buff+msg_cursor, &org_ts, sizeof(org_ts));
                msg_cursor+=sizeof(org_ts);
                memcpy(send_buff+msg_cursor, &rec_ts, sizeof(rec_ts));
                msg_cursor+=sizeof(rec_ts);

                timespec xmt_ts=get_timestamp(true);

                memcpy(send_buff+msg_cursor, &xmt_ts, sizeof(xmt_ts));
                msg_cursor+=sizeof(xmt_ts);
                msg_cursor+=sizeof(timespec); //recv_ts

                retval=sendMessage(send_buff, msg_cursor, ip, cport);
            }
            break;
        }
        case 'U':
        {
            if(verbosity>=1) eprintf("Untainting message received from %s:%d (len %d)\r\n", ip, cport, buff_len);
            if(buff_len!=U_MSG_EXPECTED_LEN)
            {
                eprintf("Untainting message received from %s:%d with unexpected length %d\r\n", ip, cport, buff_len);
                break;
            }
            else
            {
                sgx_thread_mutex_lock(&tainted_mutex);

                print_state(ENODE);

                auto sibling = std::find(siblings.begin(), siblings.end(), Peer(std::string(ip), cport));
                size_t index = std::distance(siblings.begin(), sibling);
                if(sibling != siblings.end())
                {
                    // shadow_tsc_update();
                    timespec recv_ts=ts_ref;
                    long long total_nsec = (long long)((double)(tsc-tsc_ref)/tsc_freq);
                    recv_ts+=total_nsec;
                    if(verbosity>=1) eprintf("Received untainting msg at ts:\t%.9fs\r\n", to_double(recv_ts));

                    long unsigned int offset_recv = strlen(UNTAINTING_STR);
                    const long long int recvd_tainted_msg_count = *(const long long int*)((const char*)buff + offset_recv);
                    offset_recv += sizeof(recvd_tainted_msg_count);
                    const bool views_as_consistent = *(const bool*)((const char*)buff + offset_recv);
                    offset_recv += sizeof(views_as_consistent);
                    const timespec org_ts = *(const timespec*)((const char*)buff + offset_recv);
                    offset_recv += sizeof(org_ts);
                    const timespec rec_ts = *(const timespec*)((const char*)buff + offset_recv);
                    offset_recv += sizeof(rec_ts);
                    const timespec xmt_ts = *(const timespec*)((const char*)buff + offset_recv);
                    offset_recv += sizeof(xmt_ts);

                    if (verbosity>=1)
                    {
                        eprintf("Local-Peer Timestamps:\r\n");
                        ocall_timespec_print(&org_ts, this->port, INFO);
                        ocall_timespec_print(&rec_ts, this->port, INFO);
                        ocall_timespec_print(&xmt_ts, this->port, INFO);
                        ocall_timespec_print(&recv_ts, this->port, INFO);
                        eprintf("Peer Views as consistent: %d\r\n", views_as_consistent);
                    }

                    timespec t21, t34;
                    t21=(org_ts<rec_ts)?rec_ts-org_ts:org_ts-rec_ts;
                    t34=(xmt_ts<recv_ts)?recv_ts-xmt_ts:xmt_ts-recv_ts;
                    if (verbosity>=1)
                    {
                        eprintf("t21: %.9fs t34: %.9fs\r\n", to_double(t21), to_double(t34));
                    }
                    timespec peer_clock_offset=(rec_ts-org_ts+xmt_ts-recv_ts)/2;

                    // check if peer-consistent
                    // use moving average of peer-consistencies with time discounting (i.e., a peer-consistent event raises consistency score, which then decays with time)
                    if(recvd_tainted_msg_count>sibling->last_ts_is_consistent.first)
                    {
                        siblings[index].last_ts_is_consistent=std::make_pair(recvd_tainted_msg_count, peer_clock_offset<timespec{0,500*US});
                        siblings[index].views_as_consistent=views_as_consistent;
                    }
                    else
                    {
                        eprintf("Replayed untainting message %lld from Peer %s:%d.\r\n", recvd_tainted_msg_count, siblings[index].hostname.c_str(), siblings[index].port);
                    }
                    long long int max_tainted_msg_count=recvd_tainted_msg_count;
                    for(auto& peer : siblings)
                    {
                        if(peer.last_ts_is_consistent.first>max_tainted_msg_count)
                        {
                            max_tainted_msg_count=peer.last_ts_is_consistent.first;
                        }
                    }
                    int consistent_count=0;
                    int peer_consistent_count=0;
                    for(auto& peer : siblings)
                    {
                        if(peer.last_ts_is_consistent.first==max_tainted_msg_count && peer.last_ts_is_consistent.second)
                        {
                            consistent_count++;
                        }
                        if(peer.views_as_consistent)
                        {
                            peer_consistent_count++;
                        }
                    }
                    if(consistent_count>=(int)siblings.size()-1 && peer_consistent_count>=(int)siblings.size()-1)
                    {
                        tainted = false;
                        print_state(OK);
                    }
                }
                else
                {
                    eprintf("Peer not found in siblings list.\r\n");
                }
                sgx_thread_cond_signal(&untainted_cond);
                sgx_thread_mutex_unlock(&tainted_mutex);
            }
            break;
        }
        case 'D':
        {
            if(verbosity>=1)
            {
                eprintf("Drift message received from %s:%d (len %d)\r\n", ip, cport, buff_len);
            }
            if(buff_len!=D_MSG_EXPECTED_LEN)
            {
                eprintf("Drift message received from %s:%d with unexpected length %d\r\n", ip, cport, buff_len);
                break;
            }
            else 
            {
                print_state(TA);

                //long long int mem_tsc=tsc;
                //while(mem_tsc==tsc && !should_stop());
                long unsigned int msg_cursor=strlen(DRIFT_STR);
                const long long int recvd_calib_msg_count = *(const long long int*)((const char*)buff+msg_cursor);
                msg_cursor+=sizeof(recvd_calib_msg_count);
                const int msg_sleep_time = *(const int*)((const char*)buff+msg_cursor);
                msg_cursor+=sizeof(msg_sleep_time);
                const timespec org_ts=*(const timespec*)((const char*)buff+msg_cursor);
                msg_cursor+=sizeof(org_ts);
                timespec rec_ts=*(const timespec*)((const char*)buff+msg_cursor);
                msg_cursor+=sizeof(rec_ts);
                const timespec xmt_ts=*(const timespec*)((const char*)buff+msg_cursor);
                msg_cursor+=sizeof(xmt_ts);
    
                shadow_tsc_update();

                timespec recv_ts=get_timestamp(true);

                timespec t21, t34;
                t21=(org_ts<rec_ts)?rec_ts-org_ts:org_ts-rec_ts;
                t34=(xmt_ts<recv_ts)?recv_ts-xmt_ts:xmt_ts-recv_ts;

                clock_offset=(rec_ts-org_ts+xmt_ts-recv_ts)/2;
                if(node_state==node_state::SYNC_STATE && clock_offset>=timespec{0,STEP_THRESHOLD_NS})
                {
                    node_state=node_state::SPIK_STATE;
                    print_state(callers::SPIK);
                }
                if(node_state!=node_state::FREQ_STATE)
                {
                    bool consistent=(fabs(to_double(clock_offset))<=(MAX_DRIFT_RATE*pow(2,(double)poll_exponent)));
                    static int nb_polls=0;
                    nb_polls++;
                    for(int i=0; i<3; i++)
                    {
                        double best_score=1-pow((1-alphas[i]),nb_polls); // max value if consistent variable is always true
                        if(verbosity>=1) eprintf("%.3f %.3f\r\n",best_score,pow((1-alphas[i]),nb_polls));
                        ta_consistencies[i]=exponential_moving_average(consistent,ta_consistencies[i], alphas[i]);
                        ta_consistency_scores[i]=ta_consistencies[i]/best_score;
                    }
                    if(verbosity>=1) eprintf("Consistencies: %.3f %.3f %.3f\r\n", ta_consistency_scores[0],ta_consistency_scores[1],ta_consistency_scores[2]);

                    if(ta_consistency_scores[0]<TA_CONSISTENT_THRESHOLD)
                    {
                        clock_state=clock_state::TA_INCONSISTENT_STATE;
                        print_state(callers::TA_INCONSISTENT);
                    }
                    else if (clock_state==clock_state::TA_INCONSISTENT_STATE)
                    {
                        clock_state=clock_state::TA_CONSISTENT_STATE;
                        print_state(callers::TA_CONSISTENT);
                    }
                    
                //     static double jitter=0;
                //     jitter=fabs(to_double(clock_offset))*JITTER_EMA_ALPHA+jitter*(1-JITTER_EMA_ALPHA);
                //     static long long int jiggle_counter=0;
                //     if(verbosity>=1){
                //         eprintf("Offset: %.6fs jitter: %.6fs\r\n", to_double(clock_offset),jitter);
                //     }
                //     if (fabs(to_double(clock_offset)) < CLOCK_PGATE * jitter) {
                //         jiggle_counter += poll_exponent;
                //         if (jiggle_counter > CLOCK_LIMIT) {
                //             jiggle_counter = CLOCK_LIMIT;
                //             if (poll_exponent < MAX_POLL_EXPONENT) {
                //                 jiggle_counter = 0;
                //                 poll_exponent++;
                //                 eprintf("Increased poll_exponent to %d\r\n",poll_exponent);
                //             }
                //         }
                //     } else {
                //         jiggle_counter -= poll_exponent << 1;
                //         if (jiggle_counter < -CLOCK_LIMIT) {
                //             jiggle_counter = -CLOCK_LIMIT;
                //             if (poll_exponent > MIN_POLL_EXPONENT) {
                //                 jiggle_counter = 0;
                //                 poll_exponent--;
                //                 eprintf("Decreased poll_exponent to %d\r\n",poll_exponent);
                //             }
                //         }
                //     }
                //     if(verbosity>=1){
                //         eprintf("Jiggle %lld\r\n",jiggle_counter);
                //     }
                }

                if(verbosity>=1)
                {
                    eprintf("Msg contents: %lld %.9f\r\n", recvd_calib_msg_count, to_double(xmt_ts));
                    eprintf("Offset: %.9fs\r\n",to_double(clock_offset));

                    eprintf("Local-TA Timestamps:\r\n");
                    ocall_timespec_print(&org_ts, this->port, INFO);
                    ocall_timespec_print(&rec_ts, this->port, INFO);
                    ocall_timespec_print(&xmt_ts, this->port, INFO);
                    ocall_timespec_print(&recv_ts, this->port, INFO);
                }

                eprintf("NTP state %d %.9f\r\n", node_state, to_double(clock_offset));

                select_clock();

                sgx_thread_mutex_lock(&calib_mutex);
                calib_recvd[recvd_calib_msg_count%NB_CALIB_MSG]={recvd_calib_msg_count, total_aex_count,tsc};
                sgx_thread_mutex_unlock(&calib_mutex);
            }
            break;
        }
        default:
        {
            retval=sendMessage(buff, buff_len, ip, cport);
            break;
        }
    }
    return retval;
}

void ENode::print_siblings()
{
    eprintf("%d siblings: ", siblings.size());
    for(auto& sibling: siblings)
    {
        printf("%s:%d, ", sibling.hostname.c_str(), sibling.port);
    }
    printf("\r\n");
}

bool ENode::calibrate()
{
    eprintf("Calibrating...\r\n");
    calib_count=calibrate_count();
    calibrate_drift();
    eprintf("Calibration done.\r\n");
    return true;
}

bool ENode::calibrate_count()
{
    int NB_RUNS=10;
    long long int add_count_sum = 0;
    long long int add_count_mem = add_count_sum;
    for(int i=1; i<=NB_RUNS && !should_stop(); i++)
    {
        add_count_mem = add_count_sum;
        monitor_rdtsc();
        add_count_sum += add_count;
        if(add_count_sum < add_count_mem)
        {
            eprintf("Overflow detected!\r\n");
            return false;
        }
        add_count_ref = add_count_sum/i;
    }
    return true;
}

bool ENode::calibrate_drift()
{
    send_recv_drift_message(0);
    return true;
}

void ENode::send_recv_drift_message(int sleep_time_ms, int sleep_ms)
{
    int total_sleep_time_ms=sleep_time_ms+sleep_ms;
    char send_buff[1024] = {0};
    long unsigned int msg_cursor=0;
    memcpy(send_buff+msg_cursor, DRIFT_STR, strlen(DRIFT_STR));
    msg_cursor+=strlen(DRIFT_STR);
    memcpy(send_buff+msg_cursor, &calib_msg_count, sizeof(calib_msg_count));
    msg_cursor+=sizeof(calib_msg_count);
    memcpy(send_buff+msg_cursor, &total_sleep_time_ms, sizeof(total_sleep_time_ms));
    msg_cursor+=sizeof(total_sleep_time_ms);

    shadow_tsc_update();

    timespec org_ts=get_timestamp(true);
    memcpy(send_buff+msg_cursor, &org_ts, sizeof(org_ts));
    msg_cursor+=sizeof(org_ts);
    timespec dummy_ts;
    memcpy(send_buff+msg_cursor, &dummy_ts, sizeof(dummy_ts)); // rec_ts
    msg_cursor+=sizeof(dummy_ts);
    memcpy(send_buff+msg_cursor, &dummy_ts, sizeof(dummy_ts)); // xmt_ts
    msg_cursor+=sizeof(dummy_ts);
    memcpy(send_buff+msg_cursor, &dummy_ts, sizeof(dummy_ts)); // recv_ts
    msg_cursor+=sizeof(dummy_ts);

    calib_sent[calib_msg_count%NB_CALIB_MSG]={calib_msg_count, total_aex_count, tsc};
    sendMessage(send_buff, msg_cursor, time_authority.first.c_str(), time_authority.second);
    calib_msg_count++;
}

bool ENode::monitor_rdtsc()
{
    long long int start_tsc=rdtscp();
    long long int stop_tsc=((long long int)(tsc_freq*1000000))*sleep_time+start_tsc;
#ifdef USE_AEX_NOTIFY
    #ifndef USE_SGX2
    printf("[Handler %d]> CODE DOES NOT YET SUPPORT SGX1+AEX\r\n", port);
    #else
    long long int mem_total_aex_count=total_aex_count; 
    asm volatile(
        "movq %0, %%r8\n\t"
        "movq %1, %%r9\n\t"
        "movq $0, %%r10\n\t"
        "movq %2, %%r11\n\t"

        "1: rdtscp\n\t"
        "shlq $32, %%rdx\n\t"
        "orq %%rax, %%rdx\n\t"
        "movq %%rdx, (%%r11)\n\t"
        "incq %%r10\n\t"
        "movq %%r10, (%%r8)\n\t"
        "cmpq %%r9, %%rdx\n\t"
        "jl 1b\n\t"
        :
        : "r"(&add_count), "r"(stop_tsc), "r"(&tsc)
        : "rax", "rdx", "r8", "r9"
    );
    if(total_aex_count==mem_total_aex_count)
    {
        nb_uninterr_rounds++;
    }
    else{
        nb_uninterr_rounds=0;
    }
    if(nb_uninterr_rounds>MAX_UNINTERRUPTED_ROUNDS)
    {
        if(verbosity>=1) eprintf("Self-tainting...\r\n");
        nb_uninterr_rounds=0;
        total_aex_count++;
        print_state(HANDLER);
        tainted = true;
        print_state(TAINTED);
    }
    double ACCURACY=0.05;
    if(total_aex_count==mem_total_aex_count && ((double)add_count>(double)add_count_ref*(1+ACCURACY)
    || (double)add_count<(double)add_count_ref*(1-ACCURACY)))
    {
        eprintf("Discalibrated! %f %d %f\r\n",(double)add_count_ref*(1-ACCURACY),add_count,(double)add_count_ref*(1+ACCURACY));
        calib_count = false;
        node_state=node_state::FREQ_STATE;
        print_state(callers::FREQ);    
        print_state(FULL_CALIB);
    }
    #endif
#else
    long long int delta=0;
    #ifdef USE_SGX2
    const long long int THRESHOLD=30000; // 3000 on grassen-x, 30000 on Azure
    #else
    const long long int THRESHOLD=60000;
    #endif
    do{
        long long int a=rdtscp();
        do{
            tsc=rdtscp();
            delta=tsc-a;
            a=tsc;
        } while(tsc<stop_tsc && delta<THRESHOLD && delta>0);
        if(verbosity>=1) eprintf("TSC: %lld delta: %lld\r\n", tsc, delta);
        if(delta>=THRESHOLD){
            // long long int postaex_tsc=rdtscp();
            total_aex_count++;
            nb_uninterr_rounds=0;
            tainted = true;

            // printf("[Handler %d]> AEX %lld\r\n", port, ++total_aex_count);
            print_state(HANDLER);
            if(calib_count)
            {
                print_state(TAINTED);
            }
        
            // long long int mem_tsc=tsc;
        
            // long long int total_nsec = (long long)((double)(postaex_tsc-mem_tsc)/tsc_freq);
            // printf("[Handler %d]> AEX TSC: PREAEX_TSC=%lld, POSTAEX_TSC=%lld, delta=%lld, DELTA=%lldns, TSCfreq=%.6f\r\n", port, mem_tsc, postaex_tsc, delta, total_nsec, tsc_freq);
            // if(total_nsec<0)
            // {
            //     printf("[Handler %d]> Error: Negative TSC difference detected!\r\n", port);
            // }
        }
        else if(delta<0){
            eprintf("Error: non-increasing TSC! delta=%lld\n", delta);
            nb_uninterr_rounds=0;
            calib_count = false;
            print_state(FULL_CALIB);
            break;
        }
        else
        {
            nb_uninterr_rounds++;
            if(nb_uninterr_rounds>MAX_UNINTERRUPTED_ROUNDS)
            {
                eprintf("Self-tainting...\r\n");
                nb_uninterr_rounds=0;
                total_aex_count++;
                print_state(HANDLER);
                tainted = true;
                print_state(TAINTED);
            }
        }
    } while(tsc<stop_tsc);
#endif
    return calib_count;
}

void ENode::monitor(){
    /*
    the main thread that will be called by the application.
    */ 
#ifdef USE_AEX_NOTIFY
    sgx_aex_mitigation_node_t node;

    sgx_register_aex_handler(&node, aex_handler, (const void*)&aex_args);
#endif
    ocall_timespec_get(&ts_ref);
    tsc_ref=rdtscp();
    tsc=tsc_ref;
    last_update_tsc=tsc_ref;
    
    eprintf("Real-Shadow TSC frequency: %.6f-%.6f\r\n", tsc_freq, shadow_tsc_freq);
    print_state(FULL_CALIB);

    long long int reference = 0;
    while (!should_stop())
    {    
        if(!calib_count)
        {
            calibrate();
            continue;
        }
        reference=0;
        monitor_rdtsc();

        if(verbosity>=1)
        {
            eprintf("Monitoring (%s)...\r\n", tainted?"Tainted":"Not tainted");
            printf("idx;count\r\n");
            printArray(count_aex, aex_count, reference);
            printf("%lld;%lld\r\n", aex_count, add_count-reference);
        }
        memset(count_aex, 0, sizeof(count_aex));
        memset(monitor_aex, 0, sizeof(monitor_aex));
        aex_count=0;
        monitor_aex_count=0;
        
        static bool even = false;
        if(even)
        {
            ntp();
        }
        even = !even;
    }
#ifdef USE_AEX_NOTIFY
    sgx_unregister_aex_handler(aex_handler);
#endif
    monitor_stopped = true;
    eprintf("Monitoring done.\r\n");
}

void ENode::ntp()
{
    static int poll_count=0;
    poll_count++;
    if(poll_count==(int)pow(2,(double)poll_exponent))
    {
        poll_count=0;
        send_recv_drift_message(0, sleep_attack_ms);
    }
}

void ENode::select_clock()
{
    static int pass_count=0;
    int time_constant=(int)pow(2,(double)poll_exponent);
    switch(node_state)
    {
        case node_state::SYNC_STATE:
        {
            static long long startup_count=STEP_OUT_THRESHOLD_S;

            discipline_clock();
            startup_count--;

            if(clock_offset<=timespec{0,CLOCK_FLOOR_NS})
            {
                startup_count=0;
            }
        }
        break;
        case node_state::SPIK_STATE:
        {
            pass_count+=time_constant;
            if(clock_offset<=timespec{0,CLOCK_FLOOR_NS})
            {
                pass_count=0;
                node_state=node_state::SYNC_STATE;
                print_state(callers::SYNC);
            }
            else if(pass_count>=STEP_OUT_THRESHOLD_S)
            {
                step_clock();
                pass_count=0;
                node_state=node_state::SYNC_STATE;
                print_state(callers::SYNC);
            }
        }
        break;
        case node_state::FREQ_STATE:
        {
            pass_count+=time_constant;
            static timespec train_start_ts;
            static timespec train_stop_ts;
            static bool enter_freq=true;
            if(enter_freq)
            {
                step_clock();
                train_start_ts=ts_ref;
                if(verbosity>=1) eprintf("Train start ts: %.9fs\r\n",to_double(train_start_ts));
                enter_freq=false;
            }
            else if(pass_count>=STEP_OUT_THRESHOLD_S)
            {
                // set clock frequency using offset and training duration
                timespec mem_clock_offset=clock_offset;
                train_stop_ts=ts_ref;

                double train_duration = to_double(train_stop_ts-train_start_ts);
                double ratio = train_duration/(train_duration+to_double(mem_clock_offset));
                if(verbosity>=1) 
                {
                    eprintf("Offset: %.9fs\r\n",to_double(clock_offset));
                    eprintf("Train duration: %f\r\n", train_duration);
                    eprintf("Train stop ts: %.9fs\r\n",to_double(train_stop_ts));
                    eprintf("correction ratio: %f\r\n", ratio);
                }
                tsc_freq *= ratio;
                step_clock();
                calib_count = false;
                pass_count=0;
                node_state=node_state::SYNC_STATE;
                poll_exponent=MIN_POLL_EXPONENT;
                enter_freq=true;
                eprintf("Frequency adjusted to %f\r\n", tsc_freq);

                print_state(callers::SYNC);
            }
        }
        break;
        default:
        eprintf("ERROR: unexpected node state %d\r\n",node_state);
        break;
    }
}

void ENode::print_state(callers_t caller)
{
    timespec mon_ts;
    ocall_timespec_get(&mon_ts);
    ocall_timespec_print(&mon_ts, this->port, caller);
}

void ENode::step_clock()
{
    eprintf("Stepping clock by %.9fs\r\n",to_double(clock_offset));
    ts_ref+=clock_offset;
    clock_offset={0,0};
    eprintf("New clock: %.9fs\r\n",to_double(ts_ref));
    last_update_tsc=tsc;
}

void ENode::discipline_clock()
{
    timespec mu=timespec{0,0}+(long long)((double)(tsc-last_update_tsc)/tsc_freq);
    eprintf("TSC: %lld %lld %.6f\r\n",tsc, last_update_tsc, tsc_freq);
    eprintf("Last update %.9fs ago\r\n",to_double(mu));
    if(poll_exponent>=ALLAN_INTERCEPT)
    {
        tsc_freq -= to_double(clock_offset)/(std::max(pow(2,(double)poll_exponent),to_double(mu))*CLOCK_FLL);
        eprintf("FLL-Disciplining clock frequency by: %.9fs\r\n",-to_double(clock_offset)/(std::max(pow(2,(double)poll_exponent),to_double(mu))*CLOCK_FLL));
    }
    double dtemp = 4*CLOCK_PLL* pow(2, (double)poll_exponent);
    double etemp = std::min(pow(2,ALLAN_INTERCEPT), to_double(mu));
    tsc_freq -= to_double(clock_offset)*etemp/(dtemp*dtemp);
    eprintf("PLL-Disciplining clock frequency by: %.9fs\r\n",-to_double(clock_offset)*etemp/(dtemp*dtemp));
    eprintf("Frequency adjusted to %.6f\r\n", tsc_freq);
    if(clock_offset>timespec{0,0})
    {
        step_clock();
    }
    last_update_tsc=tsc;
}

int ENode::add_sibling(std::string hostname, uint16_t _port)
{
    eprintf("Adding sibling %s:%d to node...\r\n", hostname, _port);
    if(hostname=="127.0.0.1"&&_port==port)
    {
        eprintf("Won't add self as a sibling.\r\n");
        return SUCCESS;
    }
    if(std::find(siblings.begin(), siblings.end(), Peer(hostname, _port)) != siblings.end())
    {
        eprintf("Sibling already added.\r\n");
        return SUCCESS;
    }
    siblings.emplace_back(hostname, _port);
    eprintf("Sibling added.\r\n");

    const char* buff = "Sibling";
    size_t buff_len = strlen(buff);
    unsigned char buff_enc[buff_len + crypto_aead_aes256gcm_ABYTES];
    unsigned long long buff_len_enc = buff_len + crypto_aead_aes256gcm_ABYTES;
    int retval=encrypt((const unsigned char*)buff, buff_len, buff_enc, &buff_len_enc);
    eprintf("addSibling: Encrypting message to %s:%d: %s\r\n", hostname.c_str(), _port, buff);
    if(retval)
    {
        eprintf("addSibling: Encryption failed.\r\n");
    }
    else
    {
        eprintf("addSibling: Encrypted message of len %d: %s\r\n", buff_len_enc, buff_enc);
    }
    //eprintf("sento: %d, %s, %d, %d, %s, %d\r\n", sock, buff, sizeof(buff), 0, hostname.c_str(), _port);
    ssize_t sendStatus = sendto(sock, buff_enc, buff_len_enc, 0, hostname.c_str(), INET_ADDRSTRLEN, _port);
    if (sendStatus< 0 && errno) {
        eprintf("sendStatus=%d, sending error...: %d\r\n", sendStatus, errno);
        sgx_thread_rwlock_wrlock(&socket_rwlock);
        close(sock);
        sgx_thread_rwlock_unlock(&socket_rwlock);
        return SENDING_ERROR;
    }
    return SUCCESS;
}

timespec ENode::get_timestamp(bool force)
{
    long long int mem_tsc=tsc;
    while((!force && (!calib_count || clock_state==clock_state::TA_INCONSISTENT_STATE || tainted || mem_tsc==tsc || tsc < tsc_ref)) && !should_stop());
    timespec timestamp=ts_ref;
    long long total_nsec = (long long)((double)(tsc+tsc_offset-tsc_ref)/tsc_freq);
    tsc_ref=tsc+tsc_offset;
    timestamp += total_nsec;
    ts_ref=timestamp;
    return timestamp;
}

void ENode::shadow_tsc_update()
{
    if(attacker)
    {
        // long long int start_tsc=rdtscp();
        for(int i=0; i<N_TSC_UPDATES; i++)
        {
            long long int real_diff=tsc-tsc_ref;
            long long int shadow_tsc=(long long int) ((double)real_diff*(tsc_freq/shadow_tsc_freq));
            tsc_offset=(long long int)(shadow_tsc-real_diff);
            // eprintf("Shadow TSC: %lld real diff: %lld offset: %lld\r\n", shadow_tsc, real_diff, tsc_offset);

            long long int total_nsec = (long long)((double)tsc_offset/tsc_freq);
            if(total_nsec<0)
            {
                printf("[Handler %d]> Error: Negative TSC difference detected!\r\n", port);
            }
            else if(21000+total_nsec>100000) // 100µs + AEX duration
            {
                printf("[Handler %d]> Error: Large TSC difference detected: %lldns!\r\n", port, total_nsec);
            }

            ocall_timespec_get(&ts_ref);
            tsc_ref=tsc;

            total_aex_count++;
            print_state(HANDLER);
            tainted = true;
            print_state(TAINTED);
        }
        // long long int end_tsc=rdtscp();
        // printf("[Handler %d]> TSC update took %lldns\r\n", port, end_tsc-start_tsc);
    }
}