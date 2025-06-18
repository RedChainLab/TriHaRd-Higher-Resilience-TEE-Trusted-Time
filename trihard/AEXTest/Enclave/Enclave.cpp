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
#include "Enclave_t.h"
#include <stdio.h>
#include <sgx_trts_aex.h>
#include <sgx_thread.h>
#define SIZE 65536

long long int add_count = 0;
long long int tsc = 0;

long long int aex_count = 0;
long long int monitor_aex_count = 0;

long long int count_aex[SIZE];
long long int monitor_aex[SIZE];

# define BUFSIZ  8192

typedef struct {
    int isCounting;
    int isWaiting;
    sgx_thread_cond_t startCounting;
    sgx_thread_mutex_t mutex;

}cond_struct_t;

typedef enum {
    SYSCALL_SLEEP = 0,
    O_READTSC_SLEEP = 1,
    E_READTSC_SLEEP = 2,
    C_ADDER_SLEEP = 3,
    ASM_ADDER_SLEEP = 4,
    SELF_MONITOR = 5,
    AEX_SELF_MONITOR = 6,
    AEX_ASM_SELF_MONITOR = 7
}sleep_type_t;

cond_struct_t cond = {0, 0, SGX_THREAD_COND_INITIALIZER, SGX_THREAD_MUTEX_INITIALIZER};

void t_print(const char *fmt, ...)
{
    char buf[BUFSIZ] = {'\0'};
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, BUFSIZ, fmt, ap);
    va_end(ap);
    ocall_print_string(buf);
}


inline void log_aex(long long int* arr, long long int& next_index){
    if(next_index < SIZE)
    {
        arr[next_index++] = add_count;
    }
    else
    {
        t_print("Error: Array is full\n");
    }
}

static void counter_aex_handler(const sgx_exception_info_t *info, const void * args)
{
    /*
    a custom handler that will be called when an AEX occurs, storing the number of ADD operations (performed in another thread) in a global array. This allows you to 
    know when AEX occurs (the number of ADD operations increases linearly) and how often it occurs.
    */
    (void)info;
    (void)args;
    log_aex(count_aex, aex_count);
}

static void monitor_aex_handler(const sgx_exception_info_t *info, const void * args)
{
    /*
    a custom handler that will be called when an AEX occurs, storing the number of ADD operations (performed in another thread) in a global array. This allows you to 
    know when AEX occurs (the number of ADD operations increases linearly) and how often it occurs.
    */
    (void)info;
    (void)args;
    log_aex(monitor_aex, monitor_aex_count);
}

void printArray(long long int *arr, long long int size, long long int reference){
    /*
    Print a array of size SIZE, which contains the number of ADD operations performed before each AEX occurs.
    */
    for(int i = 0; i < size ; i++){
        t_print("%d;%lld\n", i, arr[i]-reference);
    }
}

void countADD(void){
    /*
    The function that will be called in another thread to perform ADD operations.
    */
    //see_pid("countADD");
    const char* args = NULL; 
    sgx_aex_mitigation_node_t node;
    sgx_register_aex_handler(&node, counter_aex_handler, (const void*)args);
    cond_struct_t *c = &cond;
    while (!c->isCounting);
    while(c->isCounting == 1){
        add_count++; 
    }
    sgx_unregister_aex_handler(counter_aex_handler);
}

void loopOReadTSC(void){
    /*
    The function that will be called in another thread to perform ADD operations.
    */
    //see_pid("countTSC");
    const char* args = NULL; 
    sgx_aex_mitigation_node_t node;
    sgx_register_aex_handler(&node, counter_aex_handler, (const void*)args);
    cond_struct_t *c = &cond;
    while (!c->isCounting);
    while(c->isCounting){
        ocall_readTSC(&add_count);
    }
    sgx_unregister_aex_handler(counter_aex_handler);
}

inline long long int rdtsc(void){
    /*
    Read the TSC register
    */
    unsigned int lo, hi;
    __asm__ __volatile__("rdtscp" : "=a" (lo), "=d" (hi));
    //t_print("lo: %d, hi: %d\n", lo, hi);
    return ((uint64_t)hi << 32) | lo;
}

void loopEReadTSC(void){
    /*
    The function that will be called in another thread to perform ADD operations.
    */
    //see_pid("countTSC");
    const char* args = NULL; 
    sgx_aex_mitigation_node_t node;
    sgx_register_aex_handler(&node, counter_aex_handler, (const void*)args);
    cond_struct_t *c = &cond;
    while (!c->isCounting);
    while(c->isCounting){
        add_count = rdtsc();
    }
    sgx_unregister_aex_handler(counter_aex_handler);
}

void main_thread(int sleep_time, int sleep_inside_enclave, int verbosity){
    /*
    the main thread that will be called by the application.
    */
    cond_struct_t *c = &cond;
    
    const char* args = NULL; 
    sgx_aex_mitigation_node_t node;
    if(sleep_inside_enclave != SELF_MONITOR){
        if(sleep_inside_enclave == AEX_SELF_MONITOR || sleep_inside_enclave == AEX_ASM_SELF_MONITOR){
            sgx_register_aex_handler(&node, counter_aex_handler, (const void*)args);
        }
        else{
            sgx_register_aex_handler(&node, monitor_aex_handler, (const void*)args);
        }
    }
    
    long long int reference = 0;
    c->isCounting = 1;
    switch(sleep_inside_enclave){
        case SYSCALL_SLEEP:
            do
            {
                reference = add_count;
            } while(reference == 0);
            ocall_sleep(&sleep_time);
        break;
        case O_READTSC_SLEEP:
        {
            do
            {
                reference = add_count;
            } while(reference == 0);
            ocall_readTSC(&reference);
            while(tsc-reference < 3000000000*sleep_time){
                ocall_readTSC(&tsc);
            }
        }
        break;
        case E_READTSC_SLEEP:
        {
            do
            {
                reference = add_count;
            } while(reference == 0);
            reference = rdtsc();
            while(tsc-reference < 3000000000*sleep_time){
                tsc = rdtsc();
            }
        }
        break;
        case C_ADDER_SLEEP:
        {
            do
            {
                reference = add_count;
            } while(reference == 0);
            for( long long int counter = 0; counter < 529*sleep_time; counter++){
                for(int i = 0; i < 1000000; i++);
            }
        }
        break;
        case ASM_ADDER_SLEEP:
        {
            do
            {
                reference = add_count;
            } while(reference == 0);
            long long int counter = 3000000*sleep_time;
            __asm__ volatile(
                "mov %0, %%rcx\n\t"
                "mov %1, %%rax\n\t"
                "1: dec %%rax\n\t"
                "mov %%rax, (%%rcx)\n\t"
                "test %%rax, %%rax\n\t"
                "jnz 1b"
                :
                : "r"(&counter), "r"(counter)
                : "rcx", "rax"
            );
            //log_aex(count_aex, aex_count);
        }
        break;
        case SELF_MONITOR:
        {
            long long int delta=0;
            long long int THRESHOLD=600;
            reference=rdtsc();
            do{
                long long int a=rdtsc();
                do{
                    add_count=rdtsc();
                    delta=add_count-a;
                    a=add_count;
                } while(add_count-reference<3000000000*sleep_time && delta<THRESHOLD && delta>0);
                if(delta>=THRESHOLD){
                    log_aex(monitor_aex, monitor_aex_count);
                }
                else if(delta<0){
                    t_print("Error: non-increasing TSC! delta=%lld\n", delta);
                    break;
                }
            } while(add_count-reference<3000000000*sleep_time);
        }
        break;
        case AEX_SELF_MONITOR:
        {
            reference=0;
            long long int start_tsc=rdtsc();
            long long int stop_tsc=3000000*sleep_time+start_tsc;
            long long int current_tsc=reference;
            do{
                add_count++;
                current_tsc=rdtsc();
            } while(current_tsc<stop_tsc);
        }
        break;
        case AEX_ASM_SELF_MONITOR:
        {
            reference=0;
            long long int start_tsc=rdtsc();
            long long int stop_tsc=3000000*sleep_time+start_tsc;
            long long int current_tsc=reference;
            asm volatile(
                "movq %0, %%r8\n\t"
                "movq %1, %%r9\n\t"
                "movq $0, %%r10\n\t"

                "1: rdtsc\n\t"
                "shlq $32, %%rdx\n\t"
                "orq %%rax, %%rdx\n\t"
                "incq %%r10\n\t"
                "movq %%r10, (%%r8)\n\t"
                "cmpq %%r9, %%rdx\n\t"
                "jl 1b\n\t"
                :
                : "r"(&add_count), "r"(stop_tsc)
                : "rax", "rdx", "r8", "r9"
            );
        }
        break;
    }
    c->isCounting = 0;
    if(sleep_inside_enclave != SELF_MONITOR){
        if(sleep_inside_enclave == AEX_SELF_MONITOR || sleep_inside_enclave == AEX_ASM_SELF_MONITOR){
            sgx_unregister_aex_handler(counter_aex_handler);
        }
        else{
            sgx_unregister_aex_handler(monitor_aex_handler);
        }
    }

    if(verbosity>=1)
    {
        t_print("idx;count\n");
        printArray(count_aex, aex_count, reference);
    }
    if(verbosity>=2)
    {
        t_print("idx;monitor_aex_count\n");
        printArray(monitor_aex, monitor_aex_count, reference);
    }
    if(verbosity==1)
    {
        t_print("%lld;%lld\n", aex_count, add_count-reference);
    }
    if(verbosity>=2)
    {
        t_print("counter_aex_count;monitor_aex_count;final_count\n");
        t_print("%lld;%lld;%lld\n", aex_count, monitor_aex_count, add_count-reference);
    }
}