#include <iostream>
#include <thread>
#include <vector>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sodium.h>
#include <mutex>

using namespace std;

#define PORT 12340
#define DRIFT_STR "Drift"

std::vector<std::thread> threads;
std::mutex sendMutex;

void handleMsg(int serSockDes, struct sockaddr_in cliAddr, socklen_t cliAddrLen, const char* buff, ssize_t readStatus, const unsigned char* nonce, const unsigned char* key, const int netw_delay_ms) 
{
    if(netw_delay_ms > 0)
    {
        usleep(netw_delay_ms * 500);
    }
    unsigned char buff_dec[1024]; 
    unsigned long long buff_len_dec;
    
    if (crypto_aead_aes256gcm_decrypt(buff_dec, &buff_len_dec,
                                       NULL, (const unsigned char*)buff, readStatus,
                                       NULL, 0, nonce, key) != 0) {
        perror("Decryption failed\r\n");
        return;
    }
    int offset_recv = strlen(DRIFT_STR);
    const long long int recvd_calib_msg_count = *(const long long int*)((const char*)buff_dec + offset_recv);
    offset_recv += sizeof(recvd_calib_msg_count);
    const int sleep_time = *(const int*)((const char*)buff_dec + offset_recv);
    offset_recv += sizeof(sleep_time);
    const timespec org_ts = *(const timespec*)((const char*)buff_dec + offset_recv);

    //cout << "Received from " << ntohs(cliAddr.sin_port) << " calib_msg: " << recvd_calib_msg_count << " and will sleep for " << sleep_time << "ms" << endl;

    if(sleep_time < 0)
    {
        usleep(-sleep_time * 1000);
    }

    timespec rec_ts;
    timespec_get(&rec_ts, TIME_UTC);
    char ts_buff[100];
    strftime(ts_buff, sizeof ts_buff, "%D %T", gmtime(&(org_ts.tv_sec)));
    printf("Node Time: %s.%09ld UTC\n", ts_buff, org_ts.tv_nsec);
    strftime(ts_buff, sizeof ts_buff, "%D %T", gmtime(&(rec_ts.tv_sec)));
    printf("TA Time: %s.%09ld UTC\n", ts_buff, rec_ts.tv_nsec);

    int msg_cursor=0;
    memcpy(buff_dec+msg_cursor, DRIFT_STR, strlen(DRIFT_STR));
    msg_cursor+=strlen(DRIFT_STR);
    memcpy(buff_dec+msg_cursor, &recvd_calib_msg_count, sizeof(recvd_calib_msg_count));
    msg_cursor+=sizeof(recvd_calib_msg_count);
    memcpy(buff_dec+msg_cursor, &sleep_time, sizeof(sleep_time));
    msg_cursor+=sizeof(sleep_time);
    memcpy(buff_dec+msg_cursor, &org_ts, sizeof(org_ts));
    msg_cursor+=sizeof(org_ts);
    memcpy(buff_dec+msg_cursor, &rec_ts, sizeof(rec_ts));
    msg_cursor+=sizeof(rec_ts);

    timespec xmt_ts;
    timespec_get(&xmt_ts, TIME_UTC);

    memcpy(buff_dec+msg_cursor, &xmt_ts, sizeof(xmt_ts));
    msg_cursor+=sizeof(xmt_ts);
    msg_cursor+=sizeof(timespec); //recv_ts

    unsigned char buff_enc[msg_cursor + crypto_aead_aes256gcm_ABYTES];
    unsigned long long buff_len_enc;

    if (crypto_aead_aes256gcm_encrypt(buff_enc, &buff_len_enc,
                                        (unsigned char*)buff_dec, msg_cursor,
                                        NULL, 0, NULL, nonce, key) != 0) {
        perror("Encryption failed\r\n");
        return;
    }
    if(sleep_time > 0)
    {
        usleep(sleep_time * 1000);
    }
    if(netw_delay_ms > 0)
    {
        usleep(netw_delay_ms * 500);
    }
    sendMutex.lock(); 
    if (sendto(serSockDes, buff_enc, buff_len_enc, 0, (struct sockaddr*)&cliAddr, cliAddrLen) < 0) { 
        perror("sending error...\n");
    }
    sendMutex.unlock(); 
}

int main(int argc, char* argv[]) 
{
    if(argc > 2)
    {
        std::cerr << "Usage: " << argv[0] << " [<netw_delay_ms>]" << std::endl;
        return -1;
    }

    int netw_delay_ms = 0;
    if(argc == 2)
    {
        netw_delay_ms = atoi(argv[1]);
    }
    if(netw_delay_ms < 0)
    {
        std::cerr << "Error: netw_delay_ms must be greater than or equal to 0" << std::endl;
        return -1;
    }

    int serSockDes;
    struct sockaddr_in serAddr, cliAddr;
    socklen_t cliAddrLen;
    char buff[1024];
    ssize_t readStatus;

    serSockDes = socket(AF_INET, SOCK_DGRAM, 0);
    if (serSockDes < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&serAddr, 0, sizeof(serAddr));
    serAddr.sin_family = AF_INET;
    serAddr.sin_addr.s_addr = INADDR_ANY;
    serAddr.sin_port = htons(PORT);

    if (bind(serSockDes, (const struct sockaddr *)&serAddr, sizeof(serAddr)) < 0) {
        perror("bind failed");
        close(serSockDes);
        exit(EXIT_FAILURE);
    }

    unsigned char nonce[crypto_aead_aes256gcm_NPUBBYTES];
    memset(nonce, 0, sizeof(nonce));

    unsigned char key[crypto_aead_aes256gcm_KEYBYTES];
    const char* test_key = "b52c505a37d78eda5dd34f20c22540ea1b58963cf8e5bf8ffa85f9f2492505b4";
    sodium_hex2bin(key, crypto_aead_aes256gcm_KEYBYTES,
                   test_key, strlen(test_key),
                   NULL, NULL, NULL);

    while (true) {
        cliAddrLen = sizeof(cliAddr);
        readStatus = recvfrom(serSockDes, buff, sizeof(buff), 0, (struct sockaddr*)&cliAddr, &cliAddrLen);
        if (readStatus < 0) {
            perror("reading error...\n");
            close(serSockDes);
            exit(-1);
        }
        threads.push_back(std::thread(handleMsg, serSockDes, cliAddr, cliAddrLen, buff, readStatus, nonce, key, netw_delay_ms));
    }

    for (auto& th : threads) th.join();
    close(serSockDes);
    return 0;
}
