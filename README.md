# libKitsunemimiProjectNetwork

![Gitlab pipeline status](https://img.shields.io/gitlab/pipeline/kitsudaiki/libKitsunemimiProjectNetwork?label=build%20and%20test&style=flat-square)
![GitHub tag (latest SemVer)](https://img.shields.io/github/v/tag/kitsudaiki/libKitsunemimiProjectNetwork?label=version&style=flat-square)
![GitHub](https://img.shields.io/github/license/kitsudaiki/libKitsunemimiProjectNetwork?style=flat-square)
![C++Version](https://img.shields.io/badge/c%2B%2B-14-blue?style=flat-square)
![Platform](https://img.shields.io/badge/platform-Linux--x64-lightgrey?style=flat-square)

## Description

This library provides a simple session-layer-protocol, which I created for data-transfers in my projects. 

It suppots sessions, which can base on Unix-Domain-Sockets, TCP or TLS encrypted TCP.

The following messages-types are supported:

- stream-messages

	This are very simple and fast messages. For these messages, there is no additional memory allocation and the receiver of the messages gets only little parts. The data are only inside the message-ring-buffer of the socket, so the receiver have to process them instantly, or they will be overwritten. These messages are for cases, when the layer above should handle the data and wants as minimal overhead as possible.

	There are two types of stream-messages:

	- static-messages

		This type has a static size, even when only a few bytes have to be send, but because the size is static, it can be easily created on the stack, which make the preparing of the message really fast.

	- dynamic messages

		These messages are only as big as necessary, but in return the are slower in creation, because the memory has to be prepared before filling.

- standalone-messages:

	The standalone-message allocating a buffer one both sides of the transfer. It process a multi-block message-transfer by a separate thread in the backgroud, which send the data in multiple parts to the receiver. There the single parts are collected into a buffer. So with this there can also be send a much bigger amount of data. So in comparism to the stream-messages, even when the message is multiple mega-bytes big, there is only one callback on the reeiver side, which contains a pointer to the buffer with the complete message. In stream-messages, the layer above has to collect and merge the single parts of the complete message.

- request-response-messages:

	This are a special case of the standalone-messages. The request-call sends a standalone-message to the receiver with an ID and blocks the thread, which has called the request-method, until the other side sends a response-message with the ID back. The request-message returns after its release the received data. This way it can force a synchronized communication to implement for example RPC-calls.



## Build

### Requirements

name | repository | version | task
--- | --- | --- | ---
g++ | g++ | >= 6.0 | Compiler for the C++ code.
make | make | >= 4.0 | process the make-file, which is created by qmake to build the programm with g++
qmake | qt5-qmake | >= 5.0 | This package provides the tool qmake, which is similar to cmake and create the make-file for compilation.
boost-filesystem library | libboost-filesystem-dev | >= 1.6 | interactions with files and directories on the system
ssl library | libssl-dev | >= 1.1 | encryption for tls connections

Installation on Ubuntu/Debian:

```bash
sudo apt-get install g++ make qt5-qmake libboost-filesystem-dev libssl-dev
```

IMPORTANT: All my projects are only tested on Linux. 

### Kitsunemimi-repositories

Repository-Name | Version-Tag | Download-Path
--- | --- | ---
libKitsunemimiCommon | v0.15.1 |  https://github.com/kitsudaiki/libKitsunemimiCommon.git
libKitsunemimiPersistence | v0.10.0 |  https://github.com/kitsudaiki/libKitsunemimiPersistence.git
libKitsunemimiNetwork | v0.6.4 |  https://github.com/kitsudaiki/libKitsunemimiNetwork.git

HINT: These Kitsunemimi-Libraries will be downloaded and build automatically with the build-script below.

### build library

In all of my repositories you will find a `build.sh`. You only have to run this script. It doesn't required sudo, because you have to install required tool via apt, for example, by yourself. But if other projects from me are required, it download them from github and build them in the correct version too. This script is also use by the ci-pipeline, so its tested with every commit.


Run the following commands:

```
git clone https://github.com/kitsudaiki/libKitsunemimiProjectNetwork.git
cd libKitsunemimiProjectNetwork
./build.sh
cd ../result
```

It create automatic a `build` and `result` directory in the directory, where you have cloned the project. At first it build all into the `build`-directory and after all build-steps are finished, it copy the include directory from the cloned repository and the build library into the `result`-directory. So you have all in one single place.

Tested on Debian and Ubuntu. If you use Centos, Arch, etc and the build-script fails on your machine, then please write me a mail and I will try to fix the script.

## Usage

(sorry, docu comes later)

## Contributing

Please give me as many inputs as possible: Bugs, bad code style, bad documentation and so on.

## License

This project is licensed under the Apache License Version 2.0 - see the [LICENSE](LICENSE) file for details
