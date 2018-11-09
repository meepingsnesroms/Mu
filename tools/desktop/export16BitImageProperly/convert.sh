#!/bin/bash

# arg1 = in path, arg2 = out path, arg3 = array name

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd $DIR

gcc ./main.c -DIMPORT_HEADER=\"$1\" -o ./convert
chmod 777 ./convert
./convert $3 > $2
rm -f ./convert
