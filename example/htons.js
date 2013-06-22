
var raw = require ("../");

if (process.argv.length < 3) {
	console.log ("node htons <unit16>");
	process.exit (-1);
}

var uint16 = parseInt (process.argv[2]);

console.log (raw.htons (uint16));
