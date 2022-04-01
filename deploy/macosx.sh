#!/bin/bash

### Config ####
srcexe="./aquaria"
repo="$(git rev-parse --show-toplevel)"
#-------------
# This script is a complete mess and i'm sorry -- fg


if [[ x$repo == x ]]; then
    echo "Repo toplevel not found, exiting"
    exit 1
fi
deploy="$repo/deploy/macosx"
exe="$deploy/$(basename "$srcexe")"

function deploycp() {
    echo "Copying [$1] -> [$deploy/$2]..."
    cp "$1" "$deploy/$2"
}

deploycp "$srcexe" "$(basename "$exe")"

function listlibs() {
    otool -L "$1" | grep -v ':$' | cut -f 2 | sed 's/ [(].*//' | grep -v '^@'
}

function checklib() {
    if [ ! -f "$1" ]; then
        echo "Required lib [$1] does not exist"
        exit 1
    fi
}

oldsdl=$(otool -L "$exe" | grep 'SDL.*dylib' | cut -f 2 | cut -f 1 -d' ')
echo "Found SDL path: [$oldsdl]"
if [[ x$oldsdl == x ]]; then
    echo "No SDL lib found, skipping..."
elif [[ $oldsdl == @* ]]; then
    echo "Looks like the SDL path is already patched, skipping..."
    oldsdl=
else
    checklib "$oldsdl"
    newsdl="$(basename "$oldsdl" | cut -f 1 -d- | sed 's/^lib//').dylib"
    np="@executable_path/$newsdl"
    echo "Changing to: [$np]"
    install_name_tool -change "$oldsdl" "$np" "$exe"
    deploycp "$oldsdl" "$newsdl"
    # For some reason SDL2 references itself so we need to patch its ID...?
    # FIXME: check that this is correct
    install_name_tool -id "$np" "$deploy/$newsdl"
fi


oldoal=$(otool -L "$exe" | grep -i 'openal.*dylib' | cut -f 2 | cut -f 1 -d' ')
echo "Found OpenAL path: [$oldoal]"
if [[ x$oldoal == x ]]; then
    echo "No OpenAL lib found, skipping..."
elif [[ $oldoal == @* ]]; then
    echo "Looks like the OpenAL path is already patched, skipping..."
    oldoal=
else
    checklib "$oldoal"
    newoal="$(basename "$oldoal" | cut -f 1 -d- | sed 's/^lib//').dylib"
    np="@executable_path/$newoal"
    echo "Changing to: [$np]"
    install_name_tool -change "$oldoal" "$np" "$exe"
    deploycp "$oldoal" "$deploy/$newoal"
fi


echo "--- Linked libs: ---"
libs=$( (
    listlibs "$exe"
    if [[ -n "$oldsdl" ]]; then
        listlibs "$deploy/$newsdl"
    fi    
    if [[ -n "$oldoal" ]]; then
        listlibs "$deploy/$newoal"
    fi
) | sort -u )
echo "$libs"
    
echo "--------------------"
echo "Make extra sure none of these point into /opt/local or /usr/local"
echo "to ensure it'll run when shipped to other machines."

