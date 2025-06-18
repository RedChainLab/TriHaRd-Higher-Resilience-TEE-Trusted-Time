#include "Enclave_t.h"
#include "ENode.h"
#include <map>

#define ENCLAVE_MGR "[ENode Mgr]> "

extern std::map<int /*port*/, ENode*> nodes;

int ecall_init(uint16_t _port, double _tsc_freq, double _shadow_tsc_freq, int _sleep_attack_ms)
{
    printf("%sInitializing enclave node...\r\n", ENCLAVE_MGR);
    if(nodes.find(_port) != nodes.end())
    {
        printf("%sENode already exists.\r\n", ENCLAVE_MGR);
        return SOCKET_ALREADY_EXISTS;
    }
    printf("%sENode does not exist yet. Creating...\r\n", ENCLAVE_MGR);
    nodes.emplace(_port, new ENode(_port, _tsc_freq, _shadow_tsc_freq, _sleep_attack_ms));
    return SUCCESS;
}

int ecall_stop(uint16_t _port)
{
    printf("%sStopping ENode...\r\n", ENCLAVE_MGR);
    if(nodes.find(_port) == nodes.end())
    {
        printf("%sENode does not exist.\r\n", ENCLAVE_MGR);
        return SOCKET_ALREADY_EXISTS;
    }
    nodes[_port]->stop_tasks();
    printf("%sEnclave stopped.\r\n", ENCLAVE_MGR);
    return SUCCESS;
}

int ecall_destroy(uint16_t _port)
{
    printf("%sDestroying ENode...\r\n", ENCLAVE_MGR);
    if(nodes.find(_port) == nodes.end())
    {
        printf("%sENode does not exist.\r\n", ENCLAVE_MGR);
        return SOCKET_ALREADY_EXISTS;
    }
    delete nodes[_port];
    nodes.erase(_port);
    printf("%sENode destroyed.\r\n", ENCLAVE_MGR);
    return SUCCESS;
}

int ecall_monitor(uint16_t _port)
{
    printf("%sStarting enclave monitoring...\r\n", ENCLAVE_MGR);
    if(nodes.find(_port) == nodes.end())
    {
        printf("%sNode does not exist...\r\n", ENCLAVE_MGR);
        return SOCKET_ALREADY_EXISTS;
    }
    nodes[_port]->monitor();
    printf("%sEnclave monitoring finished.\r\n", ENCLAVE_MGR);
    return SUCCESS;
}

int ecall_loop_recvfrom(uint16_t _port)
{
    printf("%sStarting enclave recvfrom...\r\n", ENCLAVE_MGR);
    if(nodes.find(_port) == nodes.end())
    {
        printf("%sNode does not exist...\r\n", ENCLAVE_MGR);
        return SOCKET_ALREADY_EXISTS;
    }
    nodes[_port]->loop_recvfrom();
    printf("%sEnclave recvfrom finished.\r\n", ENCLAVE_MGR);
    return SUCCESS;
}

int ecall_refresh(uint16_t _port)
{
    printf("%sLaunching refresh...\r\n", ENCLAVE_MGR);
    if(nodes.find(_port) == nodes.end())
    {
        printf("%sNode does not exist...\r\n", ENCLAVE_MGR);
        return SOCKET_ALREADY_EXISTS;
    }
    nodes[_port]->refresh();
    printf("%sEnclave refresh finished.\r\n", ENCLAVE_MGR);
    return SUCCESS;
}

int ecall_untaint_trigger(uint16_t _port)
{
    printf("%sStarting untainting trigger...\r\n", ENCLAVE_MGR);
    if(nodes.find(_port) == nodes.end())
    {
        printf("%sNode does not exist...\r\n", ENCLAVE_MGR);
        return SOCKET_ALREADY_EXISTS;
    }
    nodes[_port]->untaint_trigger();
    printf("%sUntainting trigger finished.\r\n", ENCLAVE_MGR);
    return SUCCESS;
}

int ecall_add_sibling(uint16_t _port, const char* hostname, uint16_t port)
{
    printf("%sAdding sibling at %s:%d to node at %d...\r\n", ENCLAVE_MGR, hostname, port, _port);
    if(nodes.find(_port) == nodes.end())
    {
        printf("%sNode at %d does not exist...\r\n", ENCLAVE_MGR, _port);
        return SOCKET_ALREADY_EXISTS;
    }
    nodes[_port]->add_sibling(std::string(hostname), port);
    printf("%sSibling at %s:%d added to node at %d.\r\n", ENCLAVE_MGR, hostname, port, _port);
    return SUCCESS;
}

int ecall_get_timestamp(uint16_t _port, struct timespec* ts)
{
    //printf("%sGetting timestamp...\r\n", ENCLAVE_MGR);
    if(nodes.find(_port) == nodes.end())
    {
        printf("%sNode does not exist...\r\n", ENCLAVE_MGR);
        return SOCKET_ALREADY_EXISTS;
    }
    *ts = nodes[_port]->get_timestamp();
    //printf("%sTimestamp retrieved.\r\n", ENCLAVE_MGR);
    return SUCCESS;
}