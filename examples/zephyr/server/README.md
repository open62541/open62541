# Zephyr server example
## HOWTO
```sh
# Initialize the west workspace
cd examples/zephyr
west init -l server
west update
# Build the native sim executable
# "native_sim" may need to be changed based on your system.
# (see https://docs.zephyrproject.org/latest/boards/native/native_sim/doc/index.html#and-64bit-versions )
cd server
west build -p -b native_sim
# Setup host side ethernet interface (see
# https://github.com/zephyrproject-rtos/net-tools or
# https://docs.zephyrproject.org/latest/boards/native/native_sim/doc/index.html#peripherals)
sudo ./<path-to-net-tools>/net-setup.sh &
# Execute the OPC UA server
./build/zephyr/zephyr.exe &
# Read the answer value via python-opcua CLI
uaread -u "opc.tcp://192.0.2.10:4840" -n "ns=1;s=the.answer"
```
Expected output of uaread:
```
WARNING: Requested secure channel timeout to be 3600000ms, got 600000ms instead
42
```
## Encryption
Encryption uses the mbedTLS of the Zephyr build. Enable it in `prj.conf`:
```
CONFIG_MBEDTLS=y
CONFIG_OPEN62541_ENABLE_ENCRYPTION_MBEDTLS=y
```
`CONFIG_OPEN62541_MBEDTLS_REQUIREMENTS` (on by default) then enables the mbedTLS
options for the security policies and the certificate handling, as the mbedTLS
of the Zephyr tree names them: mbedTLS 3.6 up to Zephyr 4.3, mbedTLS 4 with
TF-PSA-Crypto from Zephyr 4.4 (`arch/zephyr/Kconfig.mbedtls`). Each of them can
still be disabled in `prj.conf`; disable `CONFIG_OPEN62541_MBEDTLS_REQUIREMENTS`
to configure mbedTLS by hand. PSA's random generator needs an entropy driver
(`CONFIG_ENTROPY_GENERATOR`).
