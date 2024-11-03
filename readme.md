!!! WARNING, documentation, including this readme, is under development :) !!!
# JSON-RPC 2.0 HTTP Server Example
## What is it?
This is a basic implementation of JSON-RPC over HTTP Server; written on C++20, but with usage of POSIX/winsock sockets and Qt MetaObject system. Fully implemnted on Linux, partially implemented on Windows.
If you are looking for implementation examples, or for a start point of your implementation, you may find it interesting
## How does it work?
### JSON-RPC Protocol
JSON-RPC 2.0 is implemented according to [this] specification(https://ftp.jsonrpc.org/specification), and transported via TCP over HTTP according to [this](https://www.jsonrpc.org/historical/json-rpc-over-http.html).
### Methods invokation
For details on supported methods go to section Methods(#TODO:add methods section). Invokation is performed by Qt's MetaObject system at runtime, using QProperty and QMetaType
### TCP layer
TCP server core made using socket() API. Together with multithreading it's designed to provide good performance with as least blocking as possible. Limits of active clients is implementation defined. port ```12345``` is used for the server
### GUI availability
For the purpose of easier and quick user-friendly approach to testing and using JSON-RPC there is a simple web-page with a form, in which you can type in Method, Parameters and Id fields of JSON-RPC request and send corresponding ```POST``` request with a submit button. This page comes as a response to ```GET``` method, so after starting Server application you can open it with any browser
## Application example
JSON-RPC protocol is useless without being applied to something. In this case, it serves as a very simple CMS for some other HTML server. With currently added methods you can modify html page: adding, editing or deleting hyperlinks, text blocks and text headers. HTML server works on port ```12346```. Modification of this server can be performed with Methods(#TODO:add methods section)
## Testing
Inside [tests](https://github.com/evenrevenn/JSON_RPC_server/tree/d3ea63de215b160ed9a3b1e3f326f9385850da01/tests) folder there is a simple python script and a bunch of .txt files, containing HTTP requests. Test cases are managed manually with commenting/uncommenting calls to test function and adding/editing request files. Test result is a responces readout, that is written in ```responce.bin``` file.
## Requirements
- CMake
- C++20 compatible compiler (tested with gcc 12 and MSVC 22)
- Qt 6.x
- Python for tests
## How to use?
- Clone the repository
- Build and install with CMake
- Launch executable from folder ```install/bin/JsonRPC```
- HTML pages would be located in folder ```install/bin/DatabaseRoot``` (saving between runs now is under development)