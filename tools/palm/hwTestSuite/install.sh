#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR

./make.sh clean
./make.sh
pilot-xfer -p usb: -i ./TstSuite.prc
