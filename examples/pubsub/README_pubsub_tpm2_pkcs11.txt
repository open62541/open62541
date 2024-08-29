1. TESTED ENVIRONMENT:
Apollo Lake processor-1.60GHz
OS - Ubuntu - 20.04.2 LTS
Kernel - 5.4.0-74-generic


2. PRE-REQUISITES:
Create AES and HMAC key in the file system and copy the same in both Publisher and Subscriber nodes.
    openssl rand 16 > aes128_sym.key
    openssl rand 32 > hmac.key


3. ENVIRONMENT SETUP:
Follow the below steps in both Publisher and Subscriber nodes.
Install required packages:
    sudo apt -y install acl autoconf autoconf-archive automake build-essential cmake doxygen gcc git iproute2 libcurl4-openssl-dev libjson-c-dev libcmocka0 libcmocka-dev libglib2.0-dev libini-config-dev libmbedtls-dev libssl-dev libsqlite3-dev libtool libyaml-dev pkg-config procps python3-pip sqlite3 udev uthash-dev

Install the TPM2 Software Stack library:
    cd ${HOME}
    git clone https://github.com/tpm2-software/tpm2-tss.git
    cd ${HOME}/tpm2-tss
    ./bootstrap
    ./configure --with-udevrulesdir=/etc/udev/rules.d --with-udevrulesprefix=70-
    make -j$(nproc)
    sudo make install
    sudo ldconfig
    sudo udevadm control --reload-rules && sudo udevadm trigger

Install the TPM2 Tools (tpm2-tools):
    sudo apt install tpm2-tools
    sudo apt install opensc

Install adaptation of PKCS#11 for TPM2 (tpm2-pkcs11):
    cd ${HOME}
    git clone https://github.com/tpm2-software/tpm2-pkcs11.git
    cd ${HOME}/tpm2-pkcs11
    git fetch origin pull/717/head:local_branch && git checkout local_branch
    ./bootstrap
    ./configure
    make -j$(nproc)
    sudo make install
    sudo ldconfig
    cp ${HOME}/tpm2-pkcs11/src/pkcs11.h /usr/include

Install the required python packages:
    cd ${HOME}/tpm2-pkcs11/tools/
    sudo pip3 install pyasn1_modules
    pip3 install .

Create an directory to create slot
    cd ${HOME}/
    mkdir pkcs11_store && cd pkcs11_store

Create a primary key
    tpm2_createprimary -c primary.ctx

Persistent primary key at handle 0x81000001
    tpm2_evictcontrol -c 0x81000001
    tpm2_evictcontrol -c primary.ctx 0x81000001

Import the AES key into the TPM
    tpm2_import -C 0x81000001 -G aes -i aes128_sym.key -u aes_key.pub -r aes_key.priv

Create a primary object in a store compatible for this key
    pid="$(tpm2_ptool init --primary-handle=0x81000001 --path=${HOME}/pkcs11_store | grep id | cut -d' ' -f 2-2)"

Create a token associated with the primary object
    tpm2_ptool addtoken --pid=$pid --sopin=123456 --userpin=123456 --label=opcua --path=${HOME}/pkcs11_store

The details of the options used in this command are:
    --pid     - The primary object id to associate with this token.
    --sopin   - The Administrator pin. This pin is used for object recovery.
    --userpin - The user pin. This pin is used for authentication of object.
    --label   - An unique label to identify the profile in use.
    --path    - The location of the store directory.

Link the AES key to the tpm2_pkcs11
    tpm2_ptool link --label=opcua --userpin=123456 --key-label=enc_key --path=${HOME}/pkcs11_store aes_key.pub aes_key.priv

Import the HMAC key into the tpm2_pkcs11
    tpm2_ptool import --label=opcua --key-label=sign_key --userpin=123456 --privkey=hmac.key --algorithm=hmac --path=${HOME}/pkcs11_store

Set environment variable for TCTI connection and path for database:
    echo 'export TPM2TOOLS_TCTI="device:/dev/tpm0"' >> .bashrc
    echo 'export TPM2_PKCS11_STORE="'${HOME}'/pkcs11_store"' >> .bashrc
    echo 'PATH=$PATH:'${HOME}'/pkcs11_store:'${HOME}'/tpm2-pkcs11/tools' >> .bashrc
    source .bashrc


4. TO BUILD AND RUN PUB/SUB APPLICATIONS:
To run PubSub applications over Ethernet in two nodes connected in peer-to-peer network
    cd open62541/
    mkdir build && cd build
    cmake -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_ENCRYPTION=MBEDTLS -DUA_ENABLE_ENCRYPTION_TPM2=ON ..
    make

The binaries are generated in build/bin/ folder
    ./bin/examples/pubsub_publish_encrypted_tpm opc.eth://<MAC_of_subscriber_node> <interface> <userpin_of_token> <slotId> <encrypt_key_label> <sign_key_label>
    ./bin/examples/pubsub_subscribe_encrypted_tpm opc.eth://<MAC_of_subscriber_node> <interface> <userpin_of_token> <slotId> <encrypt_key_label> <sign_key_label>
Eg: ./bin/examples/pubsub_publish_encrypted_tpm opc.eth://00-07-32-6b-a6-63 enp2s0 123456 1 enc_key sign_key
    ./bin/examples/pubsub_subscribe_encrypted_tpm opc.eth://00-07-32-6b-a6-63 enp2s0 123456 1 enc_key sign_key

NOTE: It is recommended to run pubsub_subscribe_encrypted_tpm application first, to avoid multiple packet receive at the start (need to be handled).
      Ignore the following warnings and error displayed during the run,
        WARNING:fapi:src/tss2-fapi/api/Fapi_List.c:226:Fapi_List_Finish() Profile of path not provisioned: /HS/SRK
        ERROR:fapi:src/tss2-fapi/api/Fapi_List.c:81:Fapi_List() ErrorCode (0x00060034) Entities_List
        WARNING: Listing FAPI token objects failed: "fapi:Provisioning was not executed."
        Please see https://github.com/tpm2-software/tpm2-pkcs11/blob/master/docs/FAPI.md for more details
        WARNING: Getting tokens from fapi backend failed.
