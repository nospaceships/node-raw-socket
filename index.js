
var events = require ("events");
var raw = require ("./build/Release/raw");
var util = require ("util");

function _expandConstantObject (object) {
	var keys = [];
	for (key in object)
		keys.push (key);
	for (var i = 0; i < keys.length; i++)
		object[object[keys[i]]] = parseInt (keys[i]);
}

var Protocol = {
	0: "None",
	1: "ICMP",
	6: "TCP",
	17: "UDP"
};

_expandConstantObject (Protocol);

for (var key in events.EventEmitter.prototype) {
  raw.SocketWrap.prototype[key] = events.EventEmitter.prototype[key];
}

function Socket (options) {
	Socket.super_.call (this);
	
	this.requests = [];
	this.buffer = new Buffer ((options && options.bufferSize)
			? options.bufferSize
			: 4096);
	this.sending = false;
	
	this.wrap = new raw.SocketWrap (
			(options && options.protocol)
					? options.protocol
					: 0);
	
	var me = this;
	this.wrap.on ("sendReady", this.onSendReady.bind (me));
	this.wrap.on ("recvReady", this.onRecvReady.bind (me));
	this.wrap.on ("error", this.onError.bind (me));
	this.wrap.on ("close", this.onClose.bind (me));
};

util.inherits (Socket, events.EventEmitter);

Socket.prototype.close = function () {
	this.wrap.close ();
	return this;
}

Socket.prototype.onClose = function () {
	this.emit ("close");
}

Socket.prototype.onError = function (error) {
	this.emit ("error", error);
	this.close ();
}

Socket.prototype.onRecvReady = function () {
	var me = this;
	try {
		this.wrap.recv (this.buffer, function (buffer, bytes, source) {
			var newBuffer = buffer.slice (0, bytes);
			me.emit ("message", newBuffer, source);
		});
	} catch (error) {
		me.emit ("error", error);
	}
}

Socket.prototype.onSendReady = function () {
	if (this.requests.length > 0) {
		var me = this;
		var req = this.requests.shift ();
		try {
			this.wrap.send (req.buffer, req.offset, req.length,
					req.address, function (bytes) {
				req.callback.call (me, null, bytes);
			});
		} catch (error) {
			me.emit ("error", error);
		}
	} else {
		if (this.sending) {
			this.wrap.stopSendReady ();
			this.sending = false;
		}
			
	}
}

Socket.prototype.send = function (buffer, offset, length, address, callback) {
	if (! callback) {
		callback = address;
		address = undefined;
	}
	
	if (length + offset > buffer.length)  {
		callback.call (this, new Error ("Buffer length '" + buffer.length
				+ "' is not large enough for the specified offset '" + offset
				+ "' plus length '" + length + "'"));
		return this;
	}
	
	var req = {
		buffer: buffer,
		offset: offset,
		length: length,
		address: address,
		callback: callback
	};
	this.requests.push (req);
	
	if (! this.sending) {
		this.wrap.startSendReady ();
		this.sending = true;
	}
	
	return this;
}

exports.createSocket = function (options) {
	return new Socket (options || {});
};

exports.Protocol = Protocol;

exports.Socket = Socket;
