#!/bin/bash

RED=$(tput setaf 1)
GREEN=$(tput setaf 2)
MAGENTA=$(tput setaf 5)
CYAN=$(tput setaf 6)
RESET=$(tput sgr0)
BOLD=$(tput bold)

VENV="${VIRTUAL_ENV}"
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

set -e

echo -e "${MAGENTA}${BOLD}*** patolette installation script${RESET}"
echo -e "${BOLD}This script will:${RESET}"
echo -e "\t 1. Set up a temporary conda environment and install build dependencies."
echo -e "\t 2. Build a Python wheel for the library."
echo -e "\t 3. Repair the wheel."
echo -e "\t 4. Install the wheel in your current venv."
read -p "${BOLD}Do you wish to continue? ${RESET}[yY/nN] " -r
echo

if [[ ! $REPLY =~ ^[Yy]$ ]]; then
  echo -e "${RED}Sad stuff, man."
  exit
fi

echo -e "${CYAN}Setting up a temporary conda environment...${RESET}"
rm -rf tmp
mkdir -p tmp
curl https://repo.anaconda.com/miniconda/Miniconda3-latest-MacOSX-arm64.sh -o tmp/miniconda.sh
bash tmp/miniconda.sh -b -u -p tmp 2>/dev/null
rm tmp/miniconda.sh

echo -e "${CYAN}Installing build dependencies...${RESET}"
./tmp/bin/conda install -n base openblas conda-forge::flann -y

echo -e "${CYAN}Building wheel...${RESET}"
python3 -m venv .tmp_venv
source .tmp_venv/bin/activate
pip install build
export PATOLETTE_LIB_DIR="${DIR}/tmp/lib"
export PATOLETTE_INCLUDE_DIR="${DIR}/tmp/include"
python3 -m build

echo -e "${CYAN}Repairing wheel...${RESET}"
pip install delocate
export DYLD_LIBRARY_PATH="${DIR}/tmp/lib"
python3 -m delocate.cmd.delocate_wheel dist/*.whl

if [ ! -z "${VENV}" ]; then
  deactivate
  source "${VENV}/bin/activate"
fi

echo -e "${CYAN}Installing wheel...${RESET}"
pip install dist/*.whl --force-reinstall

echo -e "${CYAN}Cleaning up...${RESET}"
deactivate
rm -rf tmp
rm -rf dist
rm -rf .tmp_venv

echo -e "${GREEN}Installation successful!${RESET}"
echo -e "${BOLD}To uninstall, simply run: pip uninstall patolette${RESET}"
