
var raw = require ("../");

if (process.argv.length < 3) {
	console.log ("node ntonl <unit32>");
	process.exit (-1);
}

var uint32 = parseInt (process.argv[2]);

console.log (raw.ntohl (uint32));
