# Simple C Web Server

A minimal web server written in C using the Berkeley sockets API. It implements HTTP/1.1 and serves static files over TCP on Unix-like systems, handling multiple connections at once through one thread per client.

## Features

- Native network sockets (AF_INET) over TCP.
- HTTP/1.1 headers (Content-Type and Content-Length).
- One thread per connection, so a slow client doesn't hold up the rest.
- MIME type detection based on the file extension (HTML, CSS, JS, JSON, images, plain text; anything else falls back to application/octet-stream).
- Path resolution through realpath(), rejecting any request that would resolve outside the server's root directory.
- Configurable port, either as a command-line argument or through the PORT environment variable (defaults to 8080).
- Request logging with timestamp, client IP, method, path, and status code.
- Clean shutdown on SIGINT: it closes the listening socket before exiting.
- SO_REUSEADDR enabled, so restarting the server doesn't get stuck on "address already in use".

## Project structure

- server.c: server logic, socket handling, request parsing, and the connection-accept loop.
- index.html: the file served by default for /.
- Makefile: build script.
- test.sh: smoke tests that exercise the running server with curl.

## Requirements

- GCC or Clang.
- A POSIX system (Linux, BSD) with pthreads.
- GNU Make.

## Building

To compile the binary:

make

To remove the binary and temporary files:

make clean

## Running

Start the server with:

./server

By default it listens on port 8080. To use a different port:

./server 9090

or

PORT=9090 ./server

Check it with a browser at http://localhost:8080, or with curl:

curl -i localhost:8080

## Running the tests

make test

This builds the server, starts it on a separate test port, and checks that static files, missing files, MIME types, and path traversal attempts all behave as expected.

## How it works

The server follows the standard passive-socket lifecycle:

1. Create the socket with socket().
2. Bind it to an address and port with bind().
3. Start listening with listen().
4. Accept a connection with accept().
5. Hand the connection off to a new thread, which parses the request, resolves the path, sends the response, and closes the socket.

## License

MIT.
