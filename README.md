# Messenger
Implementation of a simple multi-user messenger in C++

## Prerequisites
Before building the project, you can optionally install the following prerequisites:

- Compiler with C++17 support
- CMake (version 3.25.1 or higher)
- libboost-system-dev (version 1.74.0 or higher)
- pkg-config tool
- libpqxx (version 6.4.5 or higher)

If these dependencies are not installed, they will be automatically installed using the provided `install.sh` script.

## Installation
To install the necessary dependencies (if they are missing) and build the project, run the following command:

```bash
./install.sh --build
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
- ### Server
To run the server, execute the following command:
```bash
./_result/bin/server <port>
```
- **port** - On which you want to run server.

This will start the messaging server.
And now you can connect to this port via any utility (for example, ***client***) and send messages that will be delivered to all nodes in this network.

- ### Client
To run the client, execute the following command:
```bash
./_result/bin/client <hostname> <port>
```
- **hostname** - The IP address or domain of the server you are connecting to.
- **port** - On which the server is running.

Now you can deliver messages to the server and other connected clients.

Good luck =)
