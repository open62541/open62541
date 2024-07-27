1. TESTED ENVIRONMENT:
Apollo Lake processor-1.60GHz
OS - Ubuntu - 20.04.2 LTS
Kernel - 5.4.0-74-generic

2. ENVIRONMENT SETUP:
Follow the below steps in both Publisher and Subscriber nodes.
Install required packages:
    sudo apt -y install acl autoconf autoconf-archive automake build-essential cmake doxygen gcc git iproute2 libcurl4-openssl-dev libjson-c-dev libcmocka0 libcmocka-dev libglib2.0-dev libini-config-dev libmbedtls-dev libssl-dev libsqlite3-dev libtool libyaml-dev pkg-config procps python3-pip sqlite3 udev uthash-dev

Install the TPM2 Software Stack library:
    cd ${HOME}
    git clone https://github.com/tpm2-software/tpm2-tss.git
    cd ${HOME}/tpm2-tss
    git checkout 2.4.6
    ./bootstrap
    ./configure --with-udevrulesdir=/etc/udev/rules.d --with-udevrulesprefix=70-
    make -j$(nproc)
    sudo make install
    sudo ldconfig
    sudo udevadm control --reload-rules && sudo udevadm trigger

Install the TPM2 Tools (tpm2-tools):
    cd ${HOME}
    git clone https://github.com/tpm2-software/tpm2-tools.git
    cd tpm2-tools
    git checkout 4.3.2
    ./bootstrap
    ./configure
    make -j$(nproc)
    sudo make install
    sudo apt install opensc

Install adaptation of PKCS#11 for TPM2 (tpm2-pkcs11):
    cd ${HOME}
    git clone https://github.com/tpm2-software/tpm2-pkcs11.git
    cd ${HOME}/tpm2-pkcs11
    git checkout 1.6.0
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
    mkdir pkcs11_store

Set environment variable for TCTI connection and path for database. If TPM hardware is not installed skip the below section:
    cd ${HOME}/
    echo 'export TPM2TOOLS_TCTI="device:/dev/tpm0"' >> .bashrc
    echo 'export TPM2_PKCS11_STORE="'${HOME}'/pkcs11_store"' >> .bashrc
    echo 'PATH=$PATH:'${HOME}'/pkcs11_store:'${HOME}'/tpm2-pkcs11/tools' >> .bashrc
    source .bashrc

If TPM hardware is not available use TPM simulator to run the application.
    Install the TPM2 simulator:
        cd ${HOME}
        mkdir ibmtpm1637
        cd ibmtpm1637
        wget https://sourceforge.net/projects/ibmswtpm2/files/latest/download/ibmtpm1637.tar.gz
        tar -xzf ibmtpm1637.tar.gz
        cd src
        make -j$(nproc)

    Install the TPM2 access broker and resource manager:
        cd ${HOME}
        git clone https://github.com/tpm2-software/tpm2-abrmd.git
        cd ${HOME}/tpm2-abrmd
        git checkout 2.4.0
        sudo useradd --system --user-group tss    # User already exists. Ignore message.
        ./bootstrap
        ./configure --with-dbuspolicydir=/etc/dbus-1/system.d/ --with-systemdsystemunitdir=/lib/systemd/system --with-systemdpresetdir=/lib/systemd/system-preset --datarootdir=/usr/share
        make -j$(nproc)
        sudo make install
        sudo ldconfig
        sudo pkill -HUP dbus-daemon
        systemctl daemon-reload

    Set environment variable for TCTI connection and path for database:
        cd ${HOME}/pkcs11_store
        cp ../ibmtpm1637/src/tpm_server ${HOME}/pkcs11_store
        cd ${HOME}/
        echo 'export TPM2TOOLS_TCTI="mssim:host=localhost,port=2231"' >> .bashrc
        echo 'export TPM2_PKCS11_STORE="'${HOME}'/pkcs11_store"' >> .bashrc
        echo 'PATH=$PATH:'${HOME}'/pkcs11_store:'${HOME}'/tpm2-pkcs11/tools' >> .bashrc
        source .bashrc

    In a terminal start the TPM2 simulator using the below command:
        cd ${HOME}/pkcs11_store && tpm_server

    In another terminal start the access broker and resource manager:
        sudo tpm2-abrmd --allow-root --tcti="mssim" --logger=stdout

Creates the tpm2_pkcs11.sqlite3 database
    pid="$(tpm2_ptool init | grep id | cut -d' ' -f 2-2)"

Create a token
    tpm2_ptool addtoken --pid=$pid --sopin=123456 --userpin=123456 --label=opcua

The details of the options used in this command are:
    --pid     - The primary object id to associate with this token.
    --sopin   - The Administrator pin. This pin is used for object recovery.
    --userpin - The user pin. This pin is used for authentication of object.
    --label   - An unique label to identify the profile in use.
    --path    - The location of the store directory.

Create the AES key
    tpm2_ptool addkey --label=opcua --key-label=tpm_encrypt_key --userpin=123456 --algorithm=aes128

3. GENERATE AND ENCRYPT PUB/SUB KEYS
Verify if the TPM supports the following capbilities (https://github.com/tpm2-software/tpm2-tools/blob/master/man/tpm2_getcap.1.md):
    tpm2_getcap algorithms | grep 'aes\|cbc'
    tpm2_getcap commands | grep 'TPM2_CC_EncryptDecrypt\|TPM2_CC_VerifySignature'
    # If above capbilities are not available, the application will not execute properly

Create encryption and signing key in the file system and copy the same in both Publisher and Subscriber nodes.
    cd open62541/tools/tpm_keystore/
    openssl rand 16 > aes128_encrypt.key
    openssl rand 16 > aes128_signing.key

Seal the encryption and signing key files using the key available in TPM
    gcc cert_encrypt_tpm.c -o cert_encrypt_tpm -ltpm2_pkcs11 -lssl -lcrypto
    ./cert_encrypt_tpm -s<slotID> -p<userPin> -l<keyLable> -f<keyToBeEncrypted> -o<keyToStoreEncryptedData>
Eg: ./cert_encrypt_tpm -s1 -p123456 -ltpm_encrypt_key -faes128_encrypt.key -oaes128_encrypt_sealed.key
    ./cert_encrypt_tpm -s1 -p123456 -ltpm_encrypt_key -faes128_signing.key -oaes128_signing_sealed.key

Delete the original encryption and signing key
    rm aes128_encrypt.key
    rm aes128_signing.key

4. BUILD AND RUN PUB/SUB APPLICATION
To run PubSub applications over Ethernet in two nodes connected in peer-to-peer network
    cd ../../
    mkdir build && cd build
    cmake -DUA_BUILD_EXAMPLES=ON -DUA_ENABLE_PUBSUB=ON -DUA_ENABLE_ENCRYPTION=MBEDTLS -DUA_ENABLE_ENCRYPTION_TPM2=ON ..
    make -j$(nproc)

The binaries are generated in build/bin/ folder
    ./bin/examples/pubsub_publish_encrypted_tpm_keystore opc.eth://<MAC_of_subscriber_node> <interface> <encryption_key> <signing_key> <solt_id> <user_pin> <key_lable>
    ./bin/examples/pubsub_subscribe_encrypted_tpm_keystore opc.eth://<MAC_of_subscriber_node> <interface> <encryption_key> <signing_key> <solt_id> <user_pin> <key_lable>
Eg: ./bin/examples/pubsub_publish_encrypted_tpm_keystore opc.eth://00-07-32-6b-a6-63 enp2s0 ../tools/tpm_keystore/aes128_encrypt_sealed.key ../tools/tpm_keystore/aes128_signing_sealed.key 1 123456 tpm_encrypt_key
    ./bin/examples/pubsub_subscribe_encrypted_tpm_keystore opc.eth://00-07-32-6b-a6-63 enp2s0 ../tools/tpm_keystore/aes128_encrypt_sealed.key ../tools/tpm_keystore/aes128_signing_sealed.key 1 123456 tpm_encrypt_key

NOTE: Ignore the following warnings and error displayed during the run,
        WARNING:fapi:src/tss2-fapi/api/Fapi_List.c:226:Fapi_List_Finish() Profile of path not provisioned: /HS/SRK
        ERROR:fapi:src/tss2-fapi/api/Fapi_List.c:81:Fapi_List() ErrorCode (0x00060034) Entities_List
        WARNING: Listing FAPI token objects failed: "fapi:Provisioning was not executed."
        Please see https://github.com/tpm2-software/tpm2-pkcs11/blob/master/docs/FAPI.md for more details
        WARNING: Getting tokens from fapi backend failed.
