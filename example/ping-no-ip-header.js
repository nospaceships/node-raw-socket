
var raw = require ("../");

var options = {
	protocol: raw.Protocol.ICMP
};

var socket = raw.createSocket (options);

socket.setOption (raw.SocketLevel.IPPROTO_IP, raw.SocketOption.IP_HDRINCL,
		new Buffer ([0x00, 0x00, 0x00, 0x01]), 4);

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

// ICMP echo (ping) request (the source IP address used may not match yours)
var buffer = new Buffer ([
		0x45, 0x00, 0x00, 0x3c, 0x7c, 0x9b, 0x00, 0x00,
		0x80, 0x01, 0x39, 0x8e, 0xc0, 0xa8, 0x48, 0x53,
		0xc0, 0xa8, 0x41, 0x01, 0x08, 0x00, 0x43, 0x52,
		0x00, 0x01, 0x0a, 0x09, 0x61, 0x62, 0x63, 0x64,
		0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c,
		0x6d, 0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74,
		0x75, 0x76, 0x77, 0x61, 0x62, 0x63, 0x64, 0x65,
		0x66, 0x67, 0x68, 0x69]);

function ping () {
	target = "192.168.65.1";
	socket.send (buffer, 0, buffer.length, target, function (error, bytes) {
		if (error) {
			console.log (error.toString ());
		} else {
			console.log ("sent " + bytes + " bytes to " + target);
		}
	});

	setTimeout (ping, 1000);
}

ping ();
