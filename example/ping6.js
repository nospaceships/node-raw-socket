
var raw = require ("../");

if (process.argv.length < 5) {
	console.log ("node ping6 <target> <count> <sleep-milliseconds>");
	process.exit (-1);
}

var target = process.argv[2];
var count = parseInt (process.argv[3]);
var sleep = parseInt (process.argv[4]);

var options = {
	protocol: raw.Protocol.ICMPv6,
	generateChecksums: true,
	checksumOffset: 2,
	addressFamily: raw.AddressFamily.IPv6
};

var socket = raw.createSocket (options);

socket.on ("close", function () {
	console.log ("socket closed");
	process.exit (-1);
});

socket.on ("error", function (error) {
	console.log ("error: " + error.toString ());
	process.exit (-1);
});

socket.on ("message", function (buffer, source) {
	console.log ("received " + buffer.length + " bytes from " + source);
	console.log ("data: " + buffer.toString ("hex"));
});

// ICMPv6 echo (ping) request
var buffer = new Buffer ([
		0x80, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x09,
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
		0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
		0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x61,
		0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69]);

function ping6 () {
	for (var i = 0; i < count; i++) {
		socket.send (buffer, 0, buffer.length, target, function (error, bytes) {
			if (error) {
				console.log (error.toString ());
			} else {
				console.log ("sent " + bytes + " bytes to " + target);
			}
		});
	}
	
	setTimeout (ping6, sleep);
}

ping6 ();
