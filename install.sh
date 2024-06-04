#!/bin/bash

display_help() {
  echo "Usage: $0 [option...] {build|clear|help}" >&2
  echo
  echo "   -h, --help            display this help and exit "
  echo "   -b, --build           run CMake configure and build project"
  echo "   -c, --clear           remove build directory and files"
  echo
}

if [[ "$#" -eq 0 ]]; then
  display_help
  exit 0
fi

while [[ $# -gt 0 ]]; do
  case "$1" in
    -h | --help)
      display_help
      exit 0
      ;;
    -b | --build)
      # Check if CMake is installed
      if ! command -v cmake &> /dev/null; then
        echo "CMake is not installed. Installing..."
        sudo apt install cmake -y
      fi
      # Check if Boost is installed
      if ! command -v pkg-config &> /dev/null || ! pkg-config --exists boost; then
        echo "Boost is not installed. Installing..."
        sudo apt install libboost-all-dev -y
      fi
      cmake -B _result
      cmake --build _result
      shift
      ;;
    -c | --clear)
      if [[ -d "_result" ]]; then
        rm -rf _result
        echo "The build directory has been deleted"
      else
        echo "The build directory no longer exists"
      fi
      shift
      ;;
    *)
      echo "Error: Unknown option: $1" >&2
      echo "You can get additional information by using the \"$(basename "$0") --help\" command."
      exit 1
      ;;
  esac
done
