#!/bin/sh
#
# Aquaria PSP port
# Copyright (C) 2010 Andrew Church <achurch@achurch.org>
#
# tools/create-aquaria-dat.sh: Simple script to create the "aquaria.dat"
# package file for use on the PSP.
#

######################################

echo ""
echo "Aquaria PSP package file creation script"
echo ""

FORCE=""
if test "x$1" = "x-f"; then
    FORCE=1
    shift
fi

if test -z "$1" -o \! -d "$1"; then
    echo "Usage: $0 <Aquaria-data-directory>"
    echo ""
    echo "This script reads all files in the specified directory and its"
    echo "subdirectories, and creates a new \"aquaria.dat\" package file"
    echo "in the same directory containing those files."
    echo ""
    echo "In order to run this script, the \"build-pkg\" program must be"
    echo "present in the current directory."
    echo ""
    exit 1
fi

set -e
ORIGDIR=`pwd`
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


set -e

TMPFILE=${TMPDIR-/tmp}/build-pkg.txt
trap "rm -f '${TMPFILE}'" EXIT

echo "Building data file list..."
# We drop filenames with spaces because the build tool doesn't support them.
# The game itself does not use any data files whose names contain spaces.
(cd "${DIR}"; find * -type f -print) \
    | grep -v -i '^aquaria.dat$' \
    | grep -v ' ' \
    > "$TMPFILE"

echo "Creating ${DIR}/aquaria.dat..."
(cd "${DIR}"; "${ORIGDIR}/build-pkg" "${TMPFILE}" aquaria.dat)

echo ""
echo "The package file ${DIR}/aquaria.dat was successfully created."
echo ""
