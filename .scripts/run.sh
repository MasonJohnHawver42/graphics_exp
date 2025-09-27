#!/bin/bash

if [ $# -ne 1 ]; then
    echo "Usage: $0 <option>"
    exit 1
fi

OPTION=$1

cd gfx/build || exit
make || exit
cd ../../sbx/build || exit
make || exit
cd assets || exit

echo "here $(pwd)"

case $OPTION in
    0)
        ../../../gfx/build/bin/helloworld
        ;;
    1)
        ../../../gfx/build/bin/window
        ;;
    2)
        ../../../gfx/build/bin/cube
        ;;
    *)
        echo "Invalid option: $OPTION"
        exit 1
        ;;
esac