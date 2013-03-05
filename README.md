
# raw-socket - [homepage][homepage]

This module implements raw sockets for [Node.js][nodejs].

*This module has been created primarily to facilitate implementation of the
[net-ping][net-ping] module.*

This module is installed using [node package manager (npm)][npm]:

    # This module contains C++ source code which will be compiled
    # during installation using node-gyp.  A suitable build chain
    # must be configured before installation.
    
    npm install raw-socket

It is loaded using the `require()` function:

    var raw = require ("raw-socket");

Raw sockets can then be created, and data sent using [Node.js][nodejs]
`Buffer` objects:

    var socket = raw.createSocket ({protocol: raw.Protocol.None});

    socket.on ("message", function (buffer, source) {
        console.log ("received " + buffer.length + " bytes from " + source);
    });
    
    socket.send (buffer, 0, buffer.length, "1.1.1.1", function (error, bytes) {
        if (error)
            console.log (error.toString ());
    });

[homepage]: http://re-tool.org "Homepage"
[nodejs]: http://nodejs.org "Node.js"
[net-ping]: https://npmjs.org/package/net-ping "net-ping"
[npm]: https://npmjs.org/ "npm"

# Network Protocol Support

The raw sockets exposed by this module support IPv4 and IPv6.

Raw sockets are created using the operating systems `socket()` function, and
the socket type `SOCK_RAW` specified.

# Raw Socket Behaviour

Raw sockets behave in different ways depending on operating system and
version.  For example when the automatic IP header generation feature is
disabled some operating systems will modify the IP header included in the data
to be sent, i.e. a network card maybe performing IP checksum offload, and the
operating system may set the `Protocol` field overriding what was provided in
the data to be sent.

Some operating system versions may also restict the use of raw sockets to
privileged users.  If this is the case an exception will be thrown on socket
creation using a message similar to `Operation not permitted` (this message
is likely to be different depending on operating system version).

The appropriate operating system documentation should be consulted to
understand how raw sockets will behave before attempting to use this module.

# Keeping The [Node.js][nodejs] Event Loop Alive

This module uses the `libuv` library to integrate into the [Node.js][nodejs]
event loop - this library is also used by [Node.js][nodejs].  An underlying
 `libuv` library `poll_handle_t` event watcher is used to monitor the underlying operating system raw socket used by a socket object.

All the while a socket object exists, and the sockets `close()` method has not
been called, the raw socket will keep the [Node.js][nodejs] event loop alive
which will prevent a program from exiting.

This module exports four methods which a program can use to control this
behaviour.

The `pauseRecv()` and `pauseSend()` methods stop the underlying `poll_handle_t`
event watcher used by a socket from monitoring for readable and writeable
events.  While the `resumeRecv()` and `resumeSend()` methods start the
underlying `poll_handle_t` event watcher used by a socket allowing it to
monitor for readable and writeable events.

Each socket object also exports the `recvPaused` and `sendPaused` boolean
attributes to determine the state of the underlying `poll_handle_t` event
watcher used by a socket.

Socket creation can be expensive on some platforms, and the above methods offer an alternative to closing and deleting a socket to prevent it from keeping the
[Node.js][nodejs] event loop alive.

The [Node.js][nodejs] [net-ping][net-ping] module offers a concrete example
of using these methods.  Since [Node.js][nodejs] offers no raw socket support
this module is used to implement ICMP echo (ping) support.  Once all ping
requests have been processed by the [net-ping][net-ping] module the
`pauseRecv()` and `pauseSend()` methods are used to allow a program to exit if
required.

The following example stops the underlying `poll_handle_t` event watcher used
by a socket from generating writeable events, however since readable events
will still be watched for the program will not exit immediately:

    if (! socket.recvPaused)
        socket.pauseRecv ();

The following can the be used to resume readable events:

    if (socket.recvPaused)
        socket.resumeRecv ();

The following example stops the underlying `poll_handle_t` event watcher used
by a socket from generating both readable and writeable events, if no other
event watchers have been setup (e.g. `setTimeout()`) the program will exit.

    if (! socket.recvPaused)
        socket.pauseRecv ();
    if (! socket.sendPaused)
        socket.pauseSend ();

The following can the be used to resume both readable and writeable events:

    if (socket.recvPaused)
        socket.resumeRecv ();
    if (socket.sendPaused)
        socket.resumeSend ();

When data is sent using a sockets `send()` method the `resumeSend()` method
will be called if the sockets `sendPaused` attribute is `true`, however the
`resumeRecv()` method will not be called regardless of whether the sockets
`recvPaused` attribute is `true` or `false`.

[nodejs]: http://nodejs.org "Node.js"
[net-ping]: http://npmjs.org/package/net-ping "net-ping"

# Automatic IP Header Generation

When sending a packet over a raw socket the operating system will
automatically build an IP header and place it in the outgoing packet ahead of
the data to be sent.

This module offers the ability to disable this behavior.  When disabled IP
headers must be created and included in data sent using a raw socket.

This feature is supported on all platforms for IPv4 raw sockets by utilising
the socket option `IP_HDRINCL`.

This feature is only supported on Windows platforms for IPv6 raw sockets by
utilising the socket option `IPV6_HDRINCL` - similar options for other
platforms are yet to be discovered.

To enable this feature specify the `noIpHeader` parameter to the
`createSocket()` method exposed by this module.

This feature can be enabled and disabled after socket creation using the
`noIpHeader()` method exposed by the `Socket` class:

    socket.noIpHeader (true).send (...).noIpHeader (false);

# Automatic Checksum Generation

This module offers the ability to automatically generate checksums in C++.
This offers a performance benefit over generating checksums in JavaScript.
This can be used to generate checksums for UDP, TCP and ICMP packets, for
example.

Checksums produced by this module are 16 bits long, which falls in line with
the checksums required for UDP, TCP and ICMP.

To enable automatic checksum generation specify the `generateChecksums` and
`checksumOffset` parameters to the `createSocket()` method exposed by this
module.

For example this module can be instructed to automatically generate ICMP
checksums.  ICMP checksums are located in bytes 3 and 4 of ICMP packets.
Offsets start from 0 so 2 must be specified when creating the socket:

    var options = {
        protocol: raw.Protocol.ICMP,
        generateChecksums: true,
        checksumOffset: 2
    };

    var socket = raw.createSocket (options);

When ICMP packets are sent using the created socket a 16 bit checksum will be
generated and placed into bytes 3 and 4 before the packet is sent.

Automatic checksum generation can be disabled after socket creation using the
`generateChecksums()` method exposed by the `Socket` class:

    socket.generateChecksums (false).send (...).generateChecksums (true, 2);

# Constants

The following sections describe constants exported and used by this module.

## raw.AddressFamily

This object contains constants which can be used for the `addressFamily`
option to the `createSocket()` function exposed by this module.  This option
specifies the IP protocol version to use when creating the raw socket.

The following constants are defined in this object:

 * `IPv4` - IPv4 protocol
 * `IPv6` - IPv6 protocol

## raw.Protocol

This object contains constants which can be used for the `protocol` option to
the `createSocket()` function exposed by this module.  This option specifies
the protocol number to place in the protocol field of IP headers generated by
the operating system.

The following constants are defined in this object:

 * `None` - protocol number 0
 * `ICMP` - protocol number 1
 * `TCP` - protocol number 6
 * `UDP` - protocol number 17
 * `ICMPv6` - protocol number 58

# Using This Module

Raw sockets are represented by an instance of the `Socket` class.  This
module exports the `createSocket()` function which is used to create
instances of the `Socket` class.

## raw.createSocket ([options])

The `createSocket()` function instantiates and returns an instance of the
`Socket` class:

    // Default options
    var options = {
        addressFamily: raw.AddressFamily.IPv4,
        protocol: raw.Protocol.None,
        noIpHeader: false,
        bufferSize: 4096,
        generateChecksums: false,
        checksumOffset: 0
    };
    
    var socket = raw.createSocket (options);

The optional `options` parameter is an object, and can contain the following
items:

 * `addressFamily` - Either the constant `raw.AddressFamily.IPv4` or the
   constant `raw.AddressFamily.IPv6`, defaults to the constant
   `raw.AddressFamily.IPv4`
 * `protocol` - Either one of the constants defined in the `raw.Protocol`
   object or the protocol number to use for the socket, defaults to the
   consant `raw.Protocol.None`
 * `noIpHeader` - Either `false` or `true` to specify whether or not to	
   automatically generate IP headers, defaults to `false` meaning IP headers
   will automatically be generated by the operating system
 * `bufferSize` - Size, in bytes, of the sockets internal receive buffer,
   defaults to 4096
 * `generateChecksums` - Either `true` or `false` to enable or disable the
   automatic checksum generation feature, defaults to `false`
 * `checksumOffset` - When `generateChecksums` is `true` specifies how many
   bytes to index into the send buffer to write automatically generated
   checksums, defaults to `0`

An exception will be thrown if the underlying raw socket could not be created.
The error will be an instance of the `Error` class.

The `protocol` parameter, or its default value of the constant
`raw.Protocol.None`, will be specified in the protocol field of each IP
header.

Upon receiving packets IP headers are **NOT** removed by the operating system
when using IPv4 raw socket and IP headers in received packets will be included
in data presented by this module.  When using IPv6 raw sockets IP headers are
**NOT** included in data presented by this module.

## socket.on ("close", callback)

The `close` event is emitted by the socket when the underlying raw socket
is closed.

No arguments are passed to the callback.

The following example prints a message to the console when the socket is
closed:

    socket.on ("close", function () {
        console.log ("socket closed");
    });

## socket.on ("error", callback)

The `error` event is emitted by the socket when an error occurs sending or
receiving data.

The following arguments will be passed to the `callback` function:

 * `error` - An instance of the `Error` class, the exposed `message` attribute
   will contain a detailed error message.

The following example prints a message to the console when an error occurs,
after which the socket is closed:

    socket.on ("error", function (error) {
        console.log (error.toString ());
        socket.close ();
    });

## socket.on ("message", callback)

The `message` event is emitted by the socket when data has been received.

The following arguments will be passed to the `callback` function:

 * `buffer` - A [Node.js][nodejs] `Buffer` object containing the data
   received, the buffer will be sized to fit the data received, that is the
   `length` attribute of buffer will specify how many bytes were received
 * `address` - For IPv4 raw sockets the dotted quad formatted source IP
   address of the message, e.g `192.168.1.254`, for IPv6 raw sockets the
   compressed formatted source IP address of the message, e.g.
   `fe80::a00:27ff:fe2a:3427`

The following example prints received messages in hexadecimal to the console:

    socket.on ("message", function (buffer, address) {
        console.log ("received " + buffer.length + " bytes from " + address
                + ": " + buffer.toString ("hex"));
    });

## socket.generateChecksums (generate, offset)

The `generateChecksums()` method is used to specify whether automatic checksum
generation should be performed by the socket.

The `generate` parameter is either `true` or `false` to enable or disable the
feature.  The optional `offset` parameter specifies how many bytes to index
into the send buffer when writing the generated checksum to the send buffer.

The following example enables automatic checksum generation at offset 2
resulting in checksums being written to byte 3 and 4 of the send buffer
(offsets start from 0, meaning byte 1):

    socket.generateChecksums (true, 2);

## socket.noIpHeader (noHeader)

The `noIpHeader()` method is used to specify whether IP headers will be
provided in data to be sent, or whether the operating system should
automatically generate IP headers.

The `noHeader` parameter is either `true` to specify no IP header should be
automatically generated by the operating system, or `false` to specify that
the operating system should automatically generate IP headers.

The following example disables automatic IP header generation:

    socket.noIpHeader (true);

## socket.send (buffer, offset, length, address, callback)

The `send()` method sends data to a remote host.

The `buffer` parameter is a [Node.js][nodejs] `Buffer` object containing the
data to be sent.  The `length` parameter specifies how many bytes from
`buffer`, beginning at offset `offset`, to send.  For IPv4 raw sockets the
`address` parameter contains the dotted quad formatted IP address of the
remote host to send the data to, e.g `192.168.1.254`, for IPv6 raw sockets the
`address` parameter contains the compressed formatted IP address of the remote
host to send the data to, e.g. `fe80::a00:27ff:fe2a:3427`.  The `callback`
function is called once the data has been sent.  The following arguments will
be passed to the `callback` function:

 * `error` - Instance of the `Error` class, or `null` if no error occurred
 * `bytes` - Number of bytes sent

If the `noIpHeader` option was specified as `true` when creating the socket,
or the `noIpHeader()` method exposed by the socket has been called with a
value of `true`, the data to be sent in the `buffer` must include a IP header.

The following example sends a ICMP ping message to a remote host:

    // ICMP echo (ping) request, checksum should be ok
    var buffer = new Buffer ([
            0x08, 0x00, 0x43, 0x52, 0x00, 0x01, 0x0a, 0x09,
            0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
            0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
            0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x61,
            0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69]);

    socket.send (buffer, 0, buffer.length, target, function (error, bytes) {
        if (error)
            console.log (error.toString ());
        else
            console.log ("sent " + bytes + " bytes");
    });

# Example Programs

Example programs are included under the modules `example` directory.

# Bugs & Known Issues

None, yet!

Bug reports should be sent to <stephen.vickers.sv@gmail.com>.

# Changes

## Version 1.0.0 - 29/01/2013

 * Initial release

## Version 1.0.1 - 01/02/2013

 * Move `SOCKET_ERRNO` define from `raw.cc` to `raw.h`
 * Error in exception thrown by `SocketWrap::New` in `raw.cc` stated that two
   arguments were required, this should be one
 * Corrections to the README.md
 * Missing includes causes compilation error on some systems (maybe Node
   version dependant)

## Version 1.0.2 - 02/02/2013

 * Support automatic checksum generation

## Version 1.1.0 - 13/02/2013

 * The [net-ping][net-ping] module is now implemented so update the note about
   it in the first section of the README.md
 * Support IPv6
 * Support the `IP_HDRINCL` socket option via the `noIpHeader` option to the
   `createSocket()` function and the `noIpHeader()` method exposed by the
   `Socket` class

## Version 1.1.1 - 14/02/2013

 * IP addresses not being validated

## Version 1.1.2 - 15/02/2013

 * Default protocol option to `createSession()` was incorrect in the README.md
 * The `session.on("message")` example used `message` instead of `buffer` in
   the README.md

## Version 1.1.3 - 04/03/2013

 * `raw.Socket.onSendReady()` emit's an error when `raw.SocketWrap.send()`
   throws an exception when it should call the `req.callback` callback
 * Added the `pauseRecv()`, `resumeRecv()`, `pauseSend()` and `resumeSend()`
   methods

[net-ping]: https://npmjs.org/package/net-ping "net-ping"

# Version 1.1.4 - 05/03/2013

 * Cleanup documentation for the `pauseSend()`, `pauseRecv()`, `resumeSend()`
   and `resumeRecv()` methods in the README.md

# Roadmap

In no particular order:

 * Support some socket options
 * Enhance performance by moving the send queue into the C++ raw::SocketWrap
   class

Suggestions and requirements should be sent to <stephen.vickers.sv@gmail.com>.

# License

This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
details.

You should have received a copy of the GNU General Public License along with
this program.  If not, see
[http://www.gnu.org/licenses](http://www.gnu.org/licenses).

# Author

Stephen Vickers <stephen.vickers.sv@gmail.com>
