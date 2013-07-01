
var raw = require ("../");

if (process.argv.length < 4) {
	console.log ("node ping-set-option-ip-ttl <target> <ttl>");
	process.exit (-1);
}

var target = process.argv[2];
var ttl = parseInt (process.argv[3]);

var options = {
	protocol: raw.Protocol.ICMP
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

// ICMP echo (ping) request
var buffer = new Buffer ([
		0x08, 0x00, 0x00, 0x00, 0x00, 0x01, 0x0a, 0x09,
		0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68,
		0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 0x70,
		0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x61,
		0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69]);

raw.writeChecksum (buffer, 2, raw.createChecksum (buffer));

var socketLevel = raw.SocketLevel.IPPROTO_IP;
var socketOption = raw.SocketOption.IP_TTL;

function beforeSend () {
	socket.setOption (socketLevel, socketOption, ttl);
}

function afterSend (error, bytes) {
	if (error) {
		console.log (error.toString ());
	} else {
		console.log ("sent " + bytes + " bytes to " + target);
	}
}

function ping () {
	socket.send (buffer, 0, buffer.length, target, beforeSend, afterSend);

	setTimeout (ping, 1000);
}

ping ();
