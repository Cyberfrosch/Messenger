# Messenger
Implementation of a simple multi-user messenger in C++

## Prerequisites
Before building the project, you can optionally install the following prerequisites:

- CMake (version 3.25.1 or higher)
- Boost library (version 1.74.0 or higher)

If these dependencies are not installed, they will be automatically installed using the provided `install.sh` script.

## Installation
To install the necessary dependencies (if they are missing) and build the project, run the following command:

```bash
./install.sh [-b/--build]
```
You can also display help by running:

```bash
./install.sh --help
```
The `install.sh` script provides the following options:

- `-h`, `--help`: Display help and exit.
- `-b`, `--build`: Run CMake configure and build the project.
- `-c`, `--clear`: Remove build directory and files.

## Usage
To run the server, execute the following command:
```bash
./_result/server <port>
```
This will start the messaging server.

Now you can connect to this port via any utility and send messages that will be delivered to all nodes in this network.
