# TriHaRd deployment

This subrepository contains the scripts used to deploy TriHaRd nodes on Azure VMs.


## Dependencies & System Requirements

- [Intel Linux SGX SDK](https://github.com/intel/linux-sgx): to run the search mechanism is any SGX mode (Simulation or Hardware), the Intel SGX SDK for Linux must be installed. The SDK and the procedure for Ubuntu 24.04 are available on [Github](https://github.com/intel/linux-sgx). Gold Release 2.25 was used for the paper (`git checkout sgx_2.25` once cloned).
- An SGX-enabled machine is required to run the search mechanism in Hardware mode. 
- Intel Linux SGX PSW: to run the search mechanism is Hardware mode (using SGX hardware), the Intel SGX PSW for Linux must be installed. The PSW installation procedure for Ubuntu 24.04 is provided by [Intel's Software Installation Guide](https://download.01.org/intel-sgx/latest/linux-latest/docs/Intel_SGX_SW_Installation_Guide_for_Linux.pdf#%5B%7B%22num%22%3A16%2C%22gen%22%3A0%7D%2C%7B%22name%22%3A%22XYZ%22%7D%2C69%2C179%2C0%5D).

## On the local machine

Automatic deployment of the VM is handled via the `azure-cli`, to be installed if absent (it will also require a first-time login in that case):
``` sh
sudo apt install azure-cli
```

Configure (new) file [secrets.mk](secret.mk) with appropriate values (follow the template [secrets.mk.tmpl](secrets.mk.tmpl)).

[setup.mk](setup.mk)'s `new-*` recipes generate a .json file in [config](config), containing the node's IP address. 
This address is used by `setup-*` recipes to interact with the VM and install the work environment.
It modifies [/etc/hosts](/etc/hosts), looking for a line with the VM's name (for now..., may add the line if absent in the future,) replacing the VM's IP address with the current one in the corresponding file in [config](config).

(Until streamlining occurs,) if this is a brand-new name, add a line in [/etc/hosts](/etc/hosts) with the VM's name, i.e., `<prefix>-<node-name>` (with `<prefix>` the value of [setup.mk](setup.mk)'s `PREFIX` variable).

If the deployment starts from nothing (e.g., the resource group does not exist on Azure yet):
``` sh
make -f setup.mk new-src-group new-nsg new-vnet
```

All that remains is to execute the VM creation (`new`) and `setup` recipes (be prepared to enter your private-key's password **a lot** at the end.)
``` sh
make -f setup.mk new-<node-name> setup-<node-name>
```

For future deployments, `<action>-all` recipes perform the `<action>-<node-name>` recipe for all nodes in [config](config) (useful for multi-node deployments or cases of extreme laziness.)

## On the Azure VM

You can ssh directly to the VM using `ssh (<user>@)<prefix>-<node-name>` (if you complied with the demanded modifications in [/etc/hosts](/etc/hosts).)

To compile and test against the provided quote:
``` sh
cd ${PREFIX}qvs
make SGX_DEBUG=1
./app -quote quote.bin
```

## Cleanup (from the local machine)

You can `stop`, `restart`, `dealloc`ate, and `del`ete the VM by using the recipe `<action>-<node-name>`:
``` sh
make -f setup.mk <action>-<node-name>
```
`del-{src-group|nsg|vnet}` recipes are also available.