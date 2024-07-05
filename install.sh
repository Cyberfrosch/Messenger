#!/bin/bash

DEPENDENCIES_LIST=("cmake" "libboost-system-dev" "pkg-config" "libpqxx-dev")

display_help() {
  echo "Usage: $0 [option...] {build|clear|help}" >&2
  echo
  echo "   -h, --help            display this help and exit "
  echo "   -b, --build           run CMake configure and build project"
  echo "   -c, --clear           remove build directory and files"
  echo
}

pkg_check() {
  local pkg_name="$1"

  if ! dpkg -s "${pkg_name}" &> /dev/null; then
    echo "${pkg_name} is not installed. Installing..."
    sudo apt install "${pkg_name}" -y
  fi
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
      # Check if dependencies is installed
      for pkg in "${DEPENDENCIES_LIST[@]}"; do
        pkg_check "${pkg}"
      done
      cmake -DCMAKE_BUILD_TYPE:STRING=Debug -B _result
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
