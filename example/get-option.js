
var raw = require ("../");

if (process.argv.length < 4) {
	console.log ("node get-option <name> <option>");
	process.exit (-1);
}

var level = process.argv[2];
var option = process.argv[3];

level = raw.SocketLevel[level] || parseInt (level);
option = raw.SocketOption[option] || parseInt (option);

var options = {
	protocol: raw.Protocol.ICMP
};

var socket = raw.createSocket (options);

var buffer = new Buffer (4096);
var len = socket.getOption (level, option, buffer, buffer.length);

socket.pauseSend ().pauseRecv ();

console.log (buffer.toString ("hex", 0, len));
