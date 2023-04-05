#!/usr/bin/env bash


# Check if user is root
if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit
fi

# check if git is installed
if ! [ -x "$(command -v git)" ]; then
  echo 'Error: git is not installed.' >&2
  exit 1
fi

# check if nusvm is already installed
if [ -x "$(command -v nusmv)" ]; then
  echo 'nusvm is already installed.' >&2
  exit 0
fi

set -ex

# install NuSMV
git clone https://github.com/Zaph-x/NuSMV-patched.git
cd NuSMV-patched
sh build.sh

# Prompt user if they wish to install NuSMV to /usr/local/bin
read -p "Do you wish to install NuSMV and ltl2smv to /usr/local/bin? (y/n) " yn
case $yn in
    [Yy]* ) install -m 755 nusmv /usr/local/bin; install -m 755 ltl2smv /usr/local/bin; break;;
    [Nn]* ) exit;;
    * ) echo "Please answer yes or no.";;
esac

cd ..
rm -rf NuSMV-patched

