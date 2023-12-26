#!/bin/bash
# Copyright 2018-2023 The Hush Developers
# Released under the GPLv3

# This builds a binary called "silentzdeex"

set -e

# TODO: not ideal, hushd.exe should only be looked for on windoze
if  [ -e "zdeexd" ]; then
    echo "Found zdeexd binary"
elif [ -e "hushd.exe" ]; then
    echo "Found hushd.exe binary"
else
    echo "zdeexd could not be found!"
    echo "Either copy the binary to this dir or make a symlink."
    echo "This command will create a symlink to it if this repo is in the same directory as your hush3.git: "
    echo "ln -s ../hush3/src/zdeexd"
    echo "For windoze you should copy hushd.exe to this directory"
    exit 1
fi

# Use a modified QT project file with same build.sh
SDCONF=silentzdeex.pro ./win-build.sh $@
