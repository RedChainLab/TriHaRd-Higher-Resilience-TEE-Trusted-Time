UBUNTU_VERSION=jammy
UBUNTU_NUM_VERSION=22.04
SDK_INSTALL_PATH_PREFIX=/opt/intel/
SDK_VERSION=2.25
SDK_SUBVERSION=100.3
sudo apt-get update && sudo apt-get install -y \
        build-essential \
        ocaml \
        ocamlbuild \
        automake \
        autoconf \
        libtool \
        wget \
        python-is-python3 \
        libssl-dev \
        git \
        cmake \
        perl \
    && \
    sudo apt-get install -y \
        libssl-dev \
        libcurl4-openssl-dev \
        protobuf-compiler \
        libprotobuf-dev \
        debhelper \
        cmake \
        reprepro \
        unzip \
        pkgconf \
        libboost-dev \
        libboost-system-dev \
        libboost-thread-dev \
        lsb-release \
        libsystemd0

sudo apt-get install -y \
        msr-tools

if [ ! -f sgx_linux_x64_sdk_${SDK_VERSION}.${SDK_SUBVERSION}.bin ]
then
    wget https://download.01.org/intel-sgx/sgx-linux/${SDK_VERSION}/distro/ubuntu${UBUNTU_NUM_VERSION}-server/sgx_linux_x64_sdk_${SDK_VERSION}.${SDK_SUBVERSION}.bin
fi
chmod +x sgx_linux_x64_sdk_${SDK_VERSION}.${SDK_SUBVERSION}.bin
sudo ./sgx_linux_x64_sdk_${SDK_VERSION}.${SDK_SUBVERSION}.bin --prefix ${SDK_INSTALL_PATH_PREFIX}
### If you want to build it from sources
# git clone https://github.com/intel/linux-sgx.git && cd linux-sgx && git checkout sgx_${SDK_VERSION}
# make preparation
# if [ ${UBUNTU_NUM_VERSION} = 20.04 ] 
# then
# 	sudo cp external/toolset/ubuntu20.04/* /usr/local/bin && which ar as ld objcopy objdump ranlib
# fi
# make sdk_install_pkg
# sudo ./linux/installer/bin/sgx_linux_x64_sdk_${SDK_VERSION}.${SDK_SUBVERSION}.bin --prefix ${SDK_INSTALL_PATH_PREFIX}

if [ ${UBUNTU_NUM_VERSION} = 20.04 ] 
then
    echo deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu ${UBUNTU_VERSION} main | sudo tee /etc/apt/sources.list.d/intel-sgx.list \
    && wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | sudo apt-key add
else
    echo "deb [arch=amd64] https://download.01.org/intel-sgx/sgx_repo/ubuntu ${UBUNTU_VERSION} main" | sudo tee /etc/apt/sources.list.d/intel-sgx.list
    sudo wget https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key 
    cat intel-sgx-deb.key | sudo tee /etc/apt/keyrings/intel-sgx-keyring.asc > /dev/null
    wget -qO - https://download.01.org/intel-sgx/sgx_repo/ubuntu/intel-sgx-deb.key | sudo apt-key add # for Azure, for some reason...
fi
sudo apt-get update \
 && sudo apt-get install -y libsgx-epid libsgx-quote-ex libsgx-dcap-ql \
 && sudo apt-get install -y libsgx-urts-dbgsym libsgx-enclave-common-dbgsym libsgx-dcap-ql-dbgsym libsgx-dcap-default-qpl-dbgsym #\
#  && sudo apt-get install -y libsgx-*
echo ". ${SDK_INSTALL_PATH_PREFIX}/sgxsdk/environment" >> ~/.bashrc
echo "#define _LIBCPP_SUPPORT_SGX_SUPPORT_H" | sudo tee -a /opt/intel/sgxsdk/include/tlibc/wchar.h
sudo apt-get install libcap-dev
cd ~ && git clone https://github.com/ayeks/SGX-hardware.git && cd SGX-hardware && gcc -Wl,--no-as-needed -Wall -Wextra -Wpedantic -masm=intel -o test-sgx -lcap cpuid.c rdmsr.c xsave.c vdso.c test-sgx.c

sudo sed -i 's/^\(GRUB_CMDLINE_LINUX=\).*/\1"mitigations=off nmi_watchdog=0 nosoftlockup nohz=on nohz_full=1 kthread_cpus=0 irqaffinity=0 isolcpus=nohz,managed_irq,domain,1 tsc=nowatchdog nowatchdog rcu_nocbs=1 rcu_nocb_poll skew_tick=1 intel_pstate=disable intel_idle.max_cstate=0 processor.max_cstate=0"/' /etc/default/grub
sudo update-grub
echo "echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor" >> ~/.bashrc