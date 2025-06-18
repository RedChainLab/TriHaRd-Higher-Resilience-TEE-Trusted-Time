-include secrets.mk

SRC_GROUP?=src-group
LOCATION?=location

AUTHORIZED_IP?=IP
PUB_KEY?=pathtokey.pub
USER?=user

PREFIX?=trihard-

VNET_SUFFIX?=vnet
SUBNET_SUFFIX?=subnet
NSG_SUFFIX?=nsg

VNET_NAME=${PREFIX}${VNET_SUFFIX}

NODE_VM_TYPE?=Standard_DC4s_v3#Standard_D4s_v3
CLIENT_VM_TYPE?=Standard_B16als_v2
PRIORITY?=Spot
VM_IMAGE?=Canonical:0001-com-ubuntu-server-jammy:22_04-lts-gen2:latest #Canonical:0001-com-ubuntu-server-focal:20_04-lts-gen2:latest

src-group:
	az group create --name ${SRC_GROUP} --location ${LOCATION}

nsg:
	az network nsg create --resource-group ${SRC_GROUP} --name ${PREFIX}${NSG_SUFFIX}
	az network nsg rule create --resource-group ${SRC_GROUP} --nsg-name ${PREFIX}${NSG_SUFFIX} --name allow-ip --destination-port-ranges '22' --priority 1000 --access Allow

vnet:
	az network vnet create --name ${VNET_NAME} --resource-group ${SRC_GROUP}  --address-prefixes 10.0.0.0/16
	az network vnet subnet create --name ${PREFIX}${SUBNET_SUFFIX} --address-prefixes 10.0.0.0/24 --nsg ${PREFIX}${NSG_SUFFIX} --vnet-name ${VNET_NAME} --resource-group ${SRC_GROUP}

new-all: $(patsubst config/${PREFIX}%.json,new-%,$(wildcard config/*.json))

new-node-%:
	$(eval VM_NAME=${PREFIX}$(subst new-,,$@))
	az vm create \
		--resource-group ${SRC_GROUP} \
		--name ${VM_NAME} \
		--image ${VM_IMAGE} \
		--size ${NODE_VM_TYPE} \
		--authentication-type ssh \
		--os-disk-delete-option Delete \
		--ssh-key-value ${PUB_KEY} \
		--admin-username ${USER} \
		--nsg ${PREFIX}${NSG_SUFFIX} \
		--vnet-name ${VNET_NAME} \
		--subnet ${PREFIX}${SUBNET_SUFFIX} \
		--public-ip-sku Standard \
		--priority ${PRIORITY} \
		> config/${VM_NAME}.json	
	az vm auto-shutdown \
  		--resource-group ${SRC_GROUP} \
  		--name ${VM_NAME} \
 		--time 1800

new-client-%:
	$(eval VM_NAME=${PREFIX}$(subst new-,,$@))
	az vm create \
		--resource-group ${SRC_GROUP} \
		--name ${VM_NAME} \
		--image ${VM_IMAGE} \
		--size ${CLIENT_VM_TYPE} \
		--authentication-type ssh \
		--os-disk-delete-option Delete \
		--ssh-key-value ${PUB_KEY} \
		--admin-username ${USER} \
		--nsg ${PREFIX}${NSG_SUFFIX} \
		--vnet-name ${VNET_NAME} \
		--subnet ${PREFIX}${SUBNET_SUFFIX} \
		--public-ip-sku Standard \
		> config/${VM_NAME}.json	
	az vm auto-shutdown \
  		--resource-group ${SRC_GROUP} \
  		--name ${VM_NAME} \
 		--time 1800

setup-%:
	$(eval VM_NAME=${PREFIX}$(subst setup-,,$@))
	echo "" > ~/.ssh/known_hosts
	sleep 1
	if ! grep "${VM_NAME}" /etc/hosts; \
	then \
		echo "$(shell jq -r '.publicIpAddress' config/${VM_NAME}.json)    ${VM_NAME}" | sudo tee -a /etc/hosts; \
	else \
		sudo sed -i "s/.*${VM_NAME}/$(shell jq -r '.publicIpAddress' config/${VM_NAME}.json)    ${VM_NAME}/g" /etc/hosts; \
	fi
	sleep 1
	ssh -o StrictHostKeyChecking=no ${USER}@${VM_NAME} "cd ~ && mkdir -p ${PREFIX}deployed"
	scp -o StrictHostKeyChecking=no -r deployed/* ${USER}@${VM_NAME}:/home/${USER}/${PREFIX}deployed/
	cd ../trihard && make cleanall && scp -o StrictHostKeyChecking=no -r . ${USER}@${VM_NAME}:/home/${USER}/trihard
	ssh -o StrictHostKeyChecking=no ${USER}@${VM_NAME} "cd ~ && sh ${PREFIX}deployed/setup.sh"
	ssh -o StrictHostKeyChecking=no ${USER}@${VM_NAME} "cd ~/trihard && make deps"
del-src-group:
	az group delete --name ${SRC_GROUP} --yes

del-vnet:
	az network vnet delete --name ${VNET_NAME} --resource-group ${SRC_GROUP}

del-nsg:
	az network nsg delete --name ${PREFIX}${NSG_SUFFIX} --resource-group ${SRC_GROUP}

start-all: $(patsubst config/${PREFIX}%.json,start-%,$(wildcard config/*.json))

setup-all: $(patsubst config/${PREFIX}%.json,setup-%,$(wildcard config/*.json))

stop-all: $(patsubst config/${PREFIX}%.json,stop-%,$(wildcard config/*.json))

restart-all: $(patsubst config/${PREFIX}%.json,restart-%,$(wildcard config/*.json))

del-all: $(patsubst config/${PREFIX}%.json,del-%,$(wildcard config/*.json))

del-%:
	$(eval VM_NAME=${PREFIX}$(subst del-,,$@))
	az vm delete --name ${VM_NAME} --resource-group ${SRC_GROUP} --yes
	az network nic ip-config update --name ipconfig${VM_NAME} --nic-name ${VM_NAME}VMNic --resource-group ${SRC_GROUP} --remove publicIpAddress
	az network public-ip delete --name ${VM_NAME}PublicIP --resource-group ${SRC_GROUP}
	az network nic delete --name ${VM_NAME}VMNic --resource-group ${SRC_GROUP}

restart-%:
	$(eval VM_NAME=${PREFIX}$(subst restart-,,$@))
	az vm restart --name ${VM_NAME} \
		--resource-group ${SRC_GROUP} \
		--force

start-%:
	$(eval VM_NAME=${PREFIX}$(subst start-,,$@))
	az vm start --name ${VM_NAME} \
		--resource-group ${SRC_GROUP}

stop-%:
	$(eval VM_NAME=${PREFIX}$(subst stop-,,$@))
	az vm stop --name ${VM_NAME} \
		--resource-group ${SRC_GROUP}

.PHONY: src-group vnet nsg start-all setup-all stop-all restart-all del-vnet del-nsg del-all new-all