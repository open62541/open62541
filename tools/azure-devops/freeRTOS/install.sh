#!/usr/bin/env bash

set -e

sudo apt-get update
sudo apt install -y cmake python3-pip check libsubunit-dev wget tar git

git clone -b v3.2.3 --recursive https://github.com/espressif/esp-idf.git $IDF_PATH
cd $IDF_PATH
python -m pip install wheel
python -m pip install --user -r $IDF_PATH/requirements.txt

# See https://docs.espressif.com/projects/esp-idf/en/stable/get-started/linux-setup.html#toolchain-setup
wget -q https://dl.espressif.com/dl/xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
tar -xf xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz -C $IDF_PATH
rm xtensa-esp32-elf-linux64-1.22.0-80-g6c4433a-5.2.0.tar.gz
