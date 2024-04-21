# IRC_server
IRC server written by C++98 with Unix kqueue

## Table of contents
- [Build and Run](#build-and-run)
- [Features](#features)
    - [Not Supported](#not-supported)
    - [Supported Commands](#supported-commands)
- [Documentation](#documentation)
    - [Doxygen Documentation](#doxygen-documentation)
    - [Coding Standard](#coding-standard)

***
## Requirements
- `Unix` or `Linux` or `MacOS` system
- `clang++` or `g++`
- `make`

## Build and Run
### Unix / MacOS
```bash
$ make
$ ./ircserv <port> <password>
```
### Linux
Need to install libkqueue-dev to use kqueue on Linux system
```bash
$ sudo apt install libkqueue-dev
$ make
$ ./ircserv <port> <password>
```

## Features
Based on RFC 1459 : https://datatracker.ietf.org/doc/html/rfc1459 
### Not Supported
- Server to Server communication
- Commands not listed in below supported section
- User mode
### Supported Commands
- PASS
- NICK
- USER
- JOIN
- PRIVMSG
- PART
- QUIT
- MODE [Only channel mode[`k`/`t`/`o`/`i`/`l`]]
- KICK
- TOPIC
- INVITE

## Documentation
### Doxygen Documentation
- https://ria9993.github.io/IRC_server

### Coding Standard
- [**./CodingStandard.md**](/CodingStandard.md)

### Core Concepts
- [**./CoreConcepts.md**](/CoreConcepts.md)
