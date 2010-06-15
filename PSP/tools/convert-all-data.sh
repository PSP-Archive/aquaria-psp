#!/bin/sh
#
# Aquaria PSP port
# Copyright (C) 2010 Andrew Church <achurch@achurch.org>
#
# tools/convert-all-data.sh: Simple script to convert all Aquaria textures
# (*.png) and audio files (*.ogg) for use on the PSP.
#

######################################

# Simple function to print an "[X/N] pathname" status line.

status() {
    index=$1
    total=$2
    name=$3
    width=`echo -n $total | wc -c`
    printf "   [%*d/%d] %s\n" $width $index $total $path
}

######################################

echo ""
echo "Aquaria PSP data conversion script"
echo ""

FORCE=""
if test "x$1" = "x-f"; then
    FORCE=1
    shift
fi

if test -z "$1" -o \! -d "$1"; then
    echo "Usage: $0 <Aquaria-data-directory>"
    echo ""
    echo "This script converts all *.png texture files and *.ogg audio"
    echo "files in the given directory to the *.tex and *.mp3 formats"
    echo "preferred for the PSP port.  While it is not necessary to run"
    echo "this script, Aquaria will run very slowly if only the stock"
    echo "*.png and *.ogg files are available, so use of this script is"
    echo "highly recommended.  This script will not touch your existing"
    echo "data files, but the new *.tex and *.mp3 files will be placed"
    echo "in the same directories, so you may want to make a copy of"
    echo "your existing data files and operate on that copy instead, e.g.:"
    echo ""
    echo "    cp -a /games/aquaria /games/aquaria-psp"
    echo "    $0 /games/aquaria-psp"
    echo ""
    echo "In order to run this script, the \"pngtotex\" and \"oggtomp3\""
    echo "programs must be present in the current directory."
    echo ""
    exit 1
fi

set -e
DIR=$1

if test -z "${FORCE}"; then
    echo "Verifying target directory."
    if test \! -f "${DIR}/gfx/title/logo.png"; then
        echo >&2 "${DIR} doesn't look like an Aquaria data directory\!"
        echo >&2 "If you're absolutely sure this is the correct directory,"
        echo >&2 "run this script with the \"-f\" option:"
        echo >&2 ""
        echo >&2 "    $0 -f '${DIR}'"
        echo >&2 ""
        exit 1
    fi
fi

echo "Looking for texture and audio files."
NUM_PNG=`cd "${DIR}"; find * -name \*.png -print | wc -l`
NUM_OGG=`cd "${DIR}"; find * -name \*.ogg -print | wc -l`

echo "Converting ${NUM_PNG} textures..."
n=1
(cd "${DIR}"; find * -name \*.png -print) | while read path; do
    status ${n} ${NUM_PNG} ${path}
    ./pngtotex -8 "${DIR}/${path}"
    n=$[n+1]
done
echo "Texture conversion complete."

echo "Converting ${NUM_OGG} audio files..."
n=1
(cd "${DIR}"; find * -name \*.ogg -print) | while read path; do
    status ${n} ${NUM_OGG} ${path}
    ./oggtomp3 "${DIR}/${path}"
    n=$[n+1]
done
echo "Audio file conversion complete."

echo ""
echo "All files have been successfully converted."
echo ""
