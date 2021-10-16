#!/bin/bash -e

# Setup RC file for tx
cat << EOF > ~/.transifexrc
[https://www.transifex.com]
hostname = https://www.transifex.com
username = api
password = $TRANSIFEX_API_TOKEN
EOF


set -x

echo -e "\e[1m\e[33mBuild tools information:\e[0m"
cmake --version
gcc -v
tx --version

mkdir build && cd build
cmake .. -DENABLE_QT_TRANSLATION=ON -DGENERATE_QT_TRANSLATION=ON -DCMAKE_BUILD_TYPE=Release -DENABLE_SDL2=OFF
make translation
cd ..

cd dist/languages
tx push -s
