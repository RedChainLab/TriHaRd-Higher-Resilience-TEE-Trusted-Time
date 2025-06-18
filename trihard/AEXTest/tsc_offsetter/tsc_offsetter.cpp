#include <iostream>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <asm/msr.h>

const long long int FREQ = 3000000000;

uint64_t read_msr(int cpu, uint32_t msr) {
    int fd;
    uint64_t data;

    char msr_file_name[64];
    sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
    fd = open(msr_file_name, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    if (pread(fd, &data, sizeof data, msr) != sizeof data) {
        perror("pread");
        exit(1);
    }

    close(fd);
    return data;
}

void write_msr(int cpu, uint32_t msr, uint64_t data) {
    int fd;

    char msr_file_name[64];
    sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);
    fd = open(msr_file_name, O_WRONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    if (pwrite(fd, &data, sizeof data, msr) != sizeof data) {
        perror("pwrite");
        exit(1);
    }

    close(fd);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <target-core> <read-core> <sleep_secs>" << std::endl;
        return 1;
    }
    int target_core = std::atoi(argv[1]);
    int read_core = std::atoi(argv[2]);
    int sleep_secs = std::atoi(argv[3]);
    uint32_t msr = 0x10;  // MSR IA32_TIME_STAMP_COUNTER
    uint64_t value = read_msr(read_core, msr);
    std::cout << "MSR 0x" << std::hex << msr << " = 0x" << value << std::endl;
    write_msr(target_core, msr, value + sleep_secs*FREQ);
    return 0;
}