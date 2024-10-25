# Zephyr server example
## HOWTO
```
# Initialize the west workspace
cd examples/zephyr
west init -l server
# Build the native sim executable
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
