#ifndef RAW_CC
#define RAW_CC

#include <node.h>
#include <node_buffer.h>

#include <stdio.h>
#include <string.h>
#include "raw.h"

#ifdef _WIN32
static char errbuf[1024];
#endif
const char* raw_strerror (int code) {
#ifdef _WIN32
	if (FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM, 0, code, 0, errbuf,
			1024, NULL)) {
		return errbuf;
	} else {
		strcpy (errbuf, "Unknown error");
		return errbuf;
	}
#else
	return strerror (code);
#endif
}

static uint16_t checksum (uint16_t start_with, unsigned char *buffer,
		size_t length) {
	unsigned i;
	uint32_t sum = start_with > 0 ? ~start_with & 0xffff : 0;

	for (i = 0; i < (length & ~1U); i += 2) {
		sum += (uint16_t) ntohs (*((uint16_t *) (buffer + i)));
		if (sum > 0xffff)
			sum -= 0xffff;
	}
	if (i < length) {
		sum += buffer [i] << 8;
		if (sum > 0xffff)
			sum -= 0xffff;
	}
	
	return ~sum & 0xffff;
}

namespace raw {

static Persistent<String> CloseSymbol;
static Persistent<String> EmitSymbol;
static Persistent<String> ErrorSymbol;
static Persistent<String> RecvReadySymbol;
static Persistent<String> SendReadySymbol;

void InitAll (Handle<Object> target) {
	CloseSymbol = NODE_PSYMBOL("close");
	EmitSymbol = NODE_PSYMBOL("emit");
	ErrorSymbol = NODE_PSYMBOL("error");
	RecvReadySymbol = NODE_PSYMBOL("recvReady");
	SendReadySymbol = NODE_PSYMBOL("sendReady");

	ExportConstants (target);
	ExportFunctions (target);

	SocketWrap::Init (target);
}

NODE_MODULE(raw, InitAll)

Handle<Value> CreateChecksum (const Arguments& args) {
	HandleScope scope;
	
	if (args.Length () < 2) {
		ThrowException (Exception::Error (String::New (
				"At least one argument is required")));
		return scope.Close (args.This ());
	}

	if (! args[0]->IsUint32 ()) {
		ThrowException (Exception::TypeError (String::New (
				"Start with argument must be an unsigned integer")));
		return scope.Close (args.This ());
	}
	
	uint32_t start_with = args[0]->ToUint32 ()->Value ();

	if (start_with > 65535) {
		ThrowException (Exception::TypeError (String::New (
				"Start with argument cannot be larger than 65535")));
		return scope.Close (args.This ());
	}

	if (! node::Buffer::HasInstance (args[1])) {
		ThrowException (Exception::TypeError (String::New (
				"Buffer argument must be a node Buffer object")));
		return scope.Close (args.This ());
	}
	
	Local<Object> buffer = args[1]->ToObject ();
	char *data = node::Buffer::Data (buffer);
	size_t length = node::Buffer::Length (buffer);
	unsigned int offset = 0;
	
	if (args.Length () > 2) {
		if (! args[2]->IsUint32 ()) {
			ThrowException (Exception::TypeError (String::New (
					"Offset argument must be an unsigned integer")));
			return scope.Close (args.This ());
		}
		offset = args[2]->ToUint32 ()->Value ();
		if (offset >= length) {
			ThrowException (Exception::RangeError (String::New (
					"Offset argument must be smaller than length of the buffer")));
			return scope.Close (args.This ());
		}
	}
	
	if (args.Length () > 3) {
		if (! args[3]->IsUint32 ()) {
			ThrowException (Exception::TypeError (String::New (
					"Length argument must be an unsigned integer")));
			return scope.Close (args.This ());
		}
		unsigned int new_length = args[3]->ToUint32 ()->Value ();
		if (new_length > length) {
			ThrowException (Exception::RangeError (String::New (
					"Length argument must be smaller than length of the buffer")));
			return scope.Close (args.This ());
		}
		length = new_length;
	}
	
	uint16_t sum = checksum ((uint16_t) start_with,
			(unsigned char *) data + offset, length);

	Local<Integer> number = Integer::NewFromUnsigned (sum);
	
	return scope.Close (number);
}

Handle<Value> Htonl (const Arguments& args) {
	HandleScope scope;

	if (args.Length () < 1) {
		ThrowException (Exception::Error (String::New (
				"One arguments is required")));
		return scope.Close (args.This ());
	}

	if (! args[0]->IsUint32 ()) {
		ThrowException (Exception::TypeError (String::New (
				"Number must be a 32 unsigned integer")));
		return scope.Close (args.This ());
	}

	unsigned int number = args[0]->ToUint32 ()->Value ();
	Local<Integer> converted = Integer::NewFromUnsigned (htonl (number));

	return scope.Close (converted);
}

Handle<Value> Htons (const Arguments& args) {
	HandleScope scope;
	
	if (args.Length () < 1) {
		ThrowException (Exception::Error (String::New (
				"One arguments is required")));
		return scope.Close (args.This ());
	}

	if (! args[0]->IsUint32 ()) {
		ThrowException (Exception::TypeError (String::New (
				"Number must be a 16 unsigned integer")));
		return scope.Close (args.This ());
	}
	
	unsigned int number = args[0]->ToUint32 ()->Value ();
	if (number > 65535) {
		ThrowException (Exception::RangeError (String::New (
				"Number cannot be larger than 65535")));
		return scope.Close (args.This ());
	}
	Local<Integer> converted = Integer::NewFromUnsigned (htons (number));

	return scope.Close (converted);
}

Handle<Value> Ntohl (const Arguments& args) {
	HandleScope scope;
	
	if (args.Length () < 1) {
		ThrowException (Exception::Error (String::New (
				"One arguments is required")));
		return scope.Close (args.This ());
	}

	if (! args[0]->IsUint32 ()) {
		ThrowException (Exception::TypeError (String::New (
				"Number must be a 32 unsigned integer")));
		return scope.Close (args.This ());
	}

	unsigned int number = args[0]->ToUint32 ()->Value ();
	Local<Integer> converted = Integer::NewFromUnsigned (ntohl (number));

	return scope.Close (converted);
}

Handle<Value> Ntohs (const Arguments& args) {
	HandleScope scope;
	
	if (args.Length () < 1) {
		ThrowException (Exception::Error (String::New (
				"One arguments is required")));
		return scope.Close (args.This ());
	}

	if (! args[0]->IsUint32 ()) {
		ThrowException (Exception::TypeError (String::New (
				"Number must be a 16 unsigned integer")));
		return scope.Close (args.This ());
	}
	
	unsigned int number = args[0]->ToUint32 ()->Value ();
	if (number > 65535) {
		ThrowException (Exception::RangeError (String::New (
				"Number cannot be larger than 65535")));
		return scope.Close (args.This ());
	}
	Local<Integer> converted = Integer::NewFromUnsigned (htons (number));

	return scope.Close (converted);
}

void ExportConstants (Handle<Object> target) {
	Local<Object> socket_level = Object::New ();
	Local<Object> socket_option = Object::New ();

	target->Set (String::NewSymbol ("SocketLevel"), socket_level);
	target->Set (String::NewSymbol ("SocketOption"), socket_option);

	socket_level->Set (String::NewSymbol ("SOL_SOCKET"), Number::New (SOL_SOCKET));
	socket_level->Set (String::NewSymbol ("IPPROTO_IP"), Number::New (IPPROTO_IP));
	socket_level->Set (String::NewSymbol ("IPPROTO_IPV6"), Number::New (IPPROTO_IPV6));

	socket_option->Set (String::NewSymbol ("SO_BROADCAST"), Number::New (SO_BROADCAST));
	socket_option->Set (String::NewSymbol ("SO_RCVBUF"), Number::New (SO_RCVBUF));
	socket_option->Set (String::NewSymbol ("SO_RCVTIMEO"), Number::New (SO_RCVTIMEO));
	socket_option->Set (String::NewSymbol ("SO_SNDBUF"), Number::New (SO_SNDBUF));
	socket_option->Set (String::NewSymbol ("SO_SNDTIMEO"), Number::New (SO_SNDTIMEO));

	socket_option->Set (String::NewSymbol ("IP_HDRINCL"), Number::New (IP_HDRINCL));
	socket_option->Set (String::NewSymbol ("IP_OPTIONS"), Number::New (IP_OPTIONS));
	socket_option->Set (String::NewSymbol ("IP_TOS"), Number::New (IP_TOS));
	socket_option->Set (String::NewSymbol ("IP_TTL"), Number::New (IP_TTL));

#ifdef _WIN32
	socket_option->Set (String::NewSymbol ("IPV6_HDRINCL"), Number::New (IPV6_HDRINCL));
#endif
	socket_option->Set (String::NewSymbol ("IPV6_TTL"), Number::New (IPV6_UNICAST_HOPS));
	socket_option->Set (String::NewSymbol ("IPV6_UNICAST_HOPS"), Number::New (IPV6_UNICAST_HOPS));
	socket_option->Set (String::NewSymbol ("IPV6_V6ONLY"), Number::New (IPV6_V6ONLY));
}

void ExportFunctions (Handle<Object> target) {
	target->Set (String::NewSymbol ("createChecksum"), FunctionTemplate::New (CreateChecksum)->GetFunction ());
	
	target->Set (String::NewSymbol ("htonl"), FunctionTemplate::New (Htonl)->GetFunction ());
	target->Set (String::NewSymbol ("htons"), FunctionTemplate::New (Htons)->GetFunction ());
	target->Set (String::NewSymbol ("ntohl"), FunctionTemplate::New (Ntohl)->GetFunction ());
	target->Set (String::NewSymbol ("ntohs"), FunctionTemplate::New (Ntohs)->GetFunction ());
}

void SocketWrap::Init (Handle<Object> target) {
	HandleScope scope;
	
	Local<FunctionTemplate> tpl = FunctionTemplate::New (New);
	
	tpl->InstanceTemplate ()->SetInternalFieldCount (1);
	tpl->SetClassName (String::NewSymbol ("SocketWrap"));
	
	NODE_SET_PROTOTYPE_METHOD(tpl, "close", Close);
	NODE_SET_PROTOTYPE_METHOD(tpl, "getOption", GetOption);
	NODE_SET_PROTOTYPE_METHOD(tpl, "pause", Pause);
	NODE_SET_PROTOTYPE_METHOD(tpl, "recv", Recv);
	NODE_SET_PROTOTYPE_METHOD(tpl, "send", Send);
	NODE_SET_PROTOTYPE_METHOD(tpl, "setOption", SetOption);

	target->Set (String::NewSymbol ("SocketWrap"), tpl->GetFunction ());
}

SocketWrap::SocketWrap () {
	deconstructing_ = false;
}

SocketWrap::~SocketWrap () {
	deconstructing_ = true;
	this->CloseSocket ();
}

Handle<Value> SocketWrap::Close (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	
	socket->CloseSocket ();

	return scope.Close (args.This ());
}

void SocketWrap::CloseSocket (void) {
	HandleScope scope;
	
	if (this->poll_initialised_) {
		uv_close ((uv_handle_t *) this->poll_watcher_, OnClose);
		closesocket (this->poll_fd_);
		this->poll_fd_ = INVALID_SOCKET;
		this->poll_initialised_ = false;
	}

	Local<Value> emit = this->handle_->Get (EmitSymbol);
	Local<Function> cb = emit.As<Function> ();

	Local<Value> args[1];
	args[0] = Local<Value>::New (CloseSymbol);

	cb->Call (this->handle_, 1, args);
}

int SocketWrap::CreateSocket (void) {
	if (this->poll_initialised_)
		return 0;
	
	if ((this->poll_fd_ = socket (this->family_, SOCK_RAW, this->protocol_))
			== INVALID_SOCKET)
		return SOCKET_ERRNO;

#ifdef _WIN32
	unsigned long flag = 1;
	if (ioctlsocket (this->poll_fd_, FIONBIO, &flag) == SOCKET_ERROR)
		return SOCKET_ERRNO;
#else
	int flag = 1;
	if ((flag = fcntl (this->poll_fd_, F_GETFL, 0)) == SOCKET_ERROR)
		return SOCKET_ERRNO;
	if (fcntl (this->poll_fd_, F_SETFL, flag | O_NONBLOCK) == SOCKET_ERROR)
		return SOCKET_ERRNO;
#endif

	poll_watcher_ = new uv_poll_t;
	uv_poll_init_socket (uv_default_loop (), this->poll_watcher_,
			this->poll_fd_);
	this->poll_watcher_->data = this;
	uv_poll_start (this->poll_watcher_, UV_READABLE, IoEvent);
	
	this->poll_initialised_ = true;
	
	return 0;
}

Handle<Value> SocketWrap::GetOption (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	
	if (args.Length () < 3) {
		ThrowException (Exception::Error (String::New (
				"Three arguments are required")));
		return scope.Close (args.This ());
	}

	if (! args[0]->IsNumber ()) {
		ThrowException (Exception::TypeError (String::New (
				"Level argument must be a number")));
		return scope.Close (args.This ());
	}

	if (! args[1]->IsNumber ()) {
		ThrowException (Exception::TypeError (String::New (
				"Option argument must be a number")));
		return scope.Close (args.This ());
	}

	int level = args[0]->ToInt32 ()->Value ();
	int option = args[1]->ToInt32 ()->Value ();
	SOCKET_OPT_TYPE val = NULL;
	unsigned int ival = 0;
	SOCKET_LEN_TYPE len;

	if (! node::Buffer::HasInstance (args[2])) {
		ThrowException (Exception::TypeError (String::New (
				"Value argument must be a node Buffer object if length is "
				"provided")));
		return scope.Close (args.This ());
	}
	
	Local<Object> buffer = args[2]->ToObject ();
	val = node::Buffer::Data (buffer);

	if (! args[3]->IsInt32 ()) {
		ThrowException (Exception::TypeError (String::New (
				"Length argument must be an unsigned integer")));
		return scope.Close (args.This ());
	}

	len = (SOCKET_LEN_TYPE) node::Buffer::Length (buffer);

	int rc = getsockopt (socket->poll_fd_, level, option,
			(val ? val : (SOCKET_OPT_TYPE) &ival), &len);

	if (rc == SOCKET_ERROR) {
		ThrowException (Exception::Error (String::New (
				raw_strerror (SOCKET_ERRNO))));
		return scope.Close (args.This ());
	}
	
	Local<Number> got = Integer::NewFromUnsigned (len);
	return scope.Close (got);
}

void SocketWrap::HandleIOEvent (int status, int revents) {
	HandleScope scope;

	if (status) {
		Local<Value> emit = this->handle_->Get (EmitSymbol);
		Local<Function> cb = emit.As<Function> ();

		Local<Value> args[2];
		args[0] = Local<Value>::New (ErrorSymbol);
		args[1] = Exception::Error (String::New (
				raw_strerror (uv_last_error (uv_default_loop ()).code)));
		
		cb->Call (this->handle_, 2, args);
	} else {
		Local<Value> emit = this->handle_->Get (EmitSymbol);
		Local<Function> cb = emit.As<Function> ();

		Local<Value> args[1];
		if (revents & UV_READABLE)
			args[0] = Local<Value>::New (RecvReadySymbol);
		else
			args[0] = Local<Value>::New (SendReadySymbol);

		cb->Call (this->handle_, 1, args);
	}
}

Handle<Value> SocketWrap::New (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = new SocketWrap ();
	int rc, family = AF_INET;
	
	if (args.Length () < 1) {
		ThrowException (Exception::Error (String::New (
				"One argument is required")));
		return scope.Close (args.This ());
	}
	
	if (! args[0]->IsUint32 ()) {
		ThrowException (Exception::TypeError (String::New (
				"Protocol argument must be an unsigned integer")));
		return scope.Close (args.This ());
	} else {
		socket->protocol_ = args[0]->ToUint32 ()->Value ();
	}

	if (args.Length () > 1) {
		if (! args[1]->IsUint32 ()) {
			ThrowException (Exception::TypeError (String::New (
					"Address family argument must be an unsigned integer")));
			return scope.Close (args.This ());
		} else {
			if (args[1]->ToUint32 ()->Value () == 2)
				family = AF_INET6;
		}
	}
	
	socket->family_ = family;
	
	socket->poll_initialised_ = false;
	
	socket->no_ip_header_ = false;

	rc = socket->CreateSocket ();
	if (rc != 0) {
		ThrowException (Exception::Error (String::New (raw_strerror (rc))));
		return scope.Close (args.This ());
	}

	socket->Wrap (args.This ());

	return scope.Close (args.This ());
}

void SocketWrap::OnClose (uv_handle_t *handle) {
	delete handle;
}

Handle<Value> SocketWrap::Pause (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());

	if (args.Length () < 2) {
		ThrowException (Exception::Error (String::New (
				"Two arguments are required")));
		return scope.Close (args.This ());
	}
	
	if (! args[0]->IsBoolean ()) {
		ThrowException (Exception::TypeError (String::New (
				"Recv argument must be a boolean")));
		return scope.Close (args.This ());
	}
	bool pause_recv = args[0]->ToBoolean ()->Value ();

	if (! args[1]->IsBoolean ()) {
		ThrowException (Exception::TypeError (String::New (
				"Send argument must be a boolean")));
		return scope.Close (args.This ());
	}
	bool pause_send = args[1]->ToBoolean ()->Value ();
	
	int events = (pause_recv ? 0 : UV_READABLE)
			| (pause_send ? 0 : UV_WRITABLE);

	if (! socket->deconstructing_) {
		uv_poll_stop (socket->poll_watcher_);
		if (events)
			uv_poll_start (socket->poll_watcher_, events, IoEvent);
	}
	
	return scope.Close (args.This ());
}

Handle<Value> SocketWrap::Recv (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	Local<Object> buffer;
	sockaddr_in sin_address;
	sockaddr_in6 sin6_address;
	char addr[50];
	int rc;
#ifdef _WIN32
	int sin_length = socket->family_ == AF_INET6
			? sizeof (sin6_address)
			: sizeof (sin_address);
#else
	socklen_t sin_length = socket->family_ == AF_INET6
			? sizeof (sin6_address)
			: sizeof (sin_address);
#endif
	
	if (args.Length () < 2) {
		ThrowException (Exception::Error (String::New (
				"Five arguments are required")));
		return scope.Close (args.This ());
	}
	
	if (! node::Buffer::HasInstance (args[0])) {
		ThrowException (Exception::TypeError (String::New (
				"Buffer argument must be a node Buffer object")));
		return scope.Close (args.This ());
	} else {
		buffer = args[0]->ToObject ();
	}

	if (! args[1]->IsFunction ()) {
		ThrowException (Exception::TypeError (String::New (
				"Callback argument must be a function")));
		return scope.Close (args.This ());
	}

	rc = socket->CreateSocket ();
	if (rc != 0) {
		ThrowException (Exception::Error (String::New (raw_strerror (errno))));
		return scope.Close (args.This ());
	}

	if (socket->family_ == AF_INET6) {
		memset (&sin6_address, 0, sizeof (sin6_address));
		rc = recvfrom (socket->poll_fd_, node::Buffer::Data (buffer),
				(int) node::Buffer::Length (buffer), 0, (sockaddr *) &sin6_address,
				&sin_length);
	} else {
		memset (&sin_address, 0, sizeof (sin_address));
		rc = recvfrom (socket->poll_fd_, node::Buffer::Data (buffer),
				(int) node::Buffer::Length (buffer), 0, (sockaddr *) &sin_address,
				&sin_length);
	}
	
	if (rc == SOCKET_ERROR) {
		ThrowException (Exception::Error (String::New (raw_strerror (
				SOCKET_ERRNO))));
		return scope.Close (args.This ());
	}
	
	if (socket->family_ == AF_INET6)
		uv_ip6_name (&sin6_address, addr, 50);
	else
		uv_ip4_name (&sin_address, addr, 50);
	
	Local<Function> cb = Local<Function>::Cast (args[1]);
	const unsigned argc = 3;
	Local<Value> argv[argc];
	argv[0] = args[0];
	argv[1] = Number::New (rc);
	argv[2] = String::New (addr);
	cb->Call (Context::GetCurrent ()->Global (), argc, argv);
	
	return scope.Close (args.This ());
}

Handle<Value> SocketWrap::Send (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	Local<Object> buffer;
	uint32_t offset;
	uint32_t length;
	int rc;
	char *data;
	
	if (args.Length () < 5) {
		ThrowException (Exception::Error (String::New (
				"Five arguments are required")));
		return scope.Close (args.This ());
	}
	
	if (! node::Buffer::HasInstance (args[0])) {
		ThrowException (Exception::TypeError (String::New (
				"Buffer argument must be a node Buffer object")));
		return scope.Close (args.This ());
	}
	
	if (! args[1]->IsUint32 ()) {
		ThrowException (Exception::TypeError (String::New (
				"Offset argument must be an unsigned integer")));
		return scope.Close (args.This ());
	}

	if (! args[2]->IsUint32 ()) {
		ThrowException (Exception::TypeError (String::New (
				"Length argument must be an unsigned integer")));
		return scope.Close (args.This ());
	}

	if (! args[3]->IsString ()) {
		ThrowException (Exception::TypeError (String::New (
				"Address argument must be a string")));
		return scope.Close (args.This ());
	}

	if (! args[4]->IsFunction ()) {
		ThrowException (Exception::TypeError (String::New (
				"Callback argument must be a function")));
		return scope.Close (args.This ());
	}

	rc = socket->CreateSocket ();
	if (rc != 0) {
		ThrowException (Exception::Error (String::New (raw_strerror (errno))));
		return scope.Close (args.This ());
	}
	
	buffer = args[0]->ToObject ();
	offset = args[1]->ToUint32 ()->Value ();
	length = args[2]->ToUint32 ()->Value ();
	String::AsciiValue address (args[3]);

	data = node::Buffer::Data (buffer) + offset;
	
	if (socket->family_ == AF_INET6) {
		struct sockaddr_in6 addr = uv_ip6_addr (*address, 0);
		rc = sendto (socket->poll_fd_, data, length, 0,
				(struct sockaddr *) &addr, sizeof (addr));
	} else {
		struct sockaddr_in addr = uv_ip4_addr (*address, 0);
		rc = sendto (socket->poll_fd_, data, length, 0,
				(struct sockaddr *) &addr, sizeof (addr));
	}
	
	if (rc == SOCKET_ERROR) {
		ThrowException (Exception::Error (String::New (raw_strerror (
				SOCKET_ERRNO))));
		return scope.Close (args.This ());
	}
	
	Local<Function> cb = Local<Function>::Cast (args[4]);
	const unsigned argc = 1;
	Local<Value> argv[argc];
	argv[0] = Number::New (rc);
	cb->Call (Context::GetCurrent ()->Global (), argc, argv);
	
	return scope.Close (args.This ());
}

Handle<Value> SocketWrap::SetOption (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	
	if (args.Length () < 3) {
		ThrowException (Exception::Error (String::New (
				"Three or four arguments are required")));
		return scope.Close (args.This ());
	}

	if (! args[0]->IsNumber ()) {
		ThrowException (Exception::TypeError (String::New (
				"Level argument must be a number")));
		return scope.Close (args.This ());
	}

	if (! args[1]->IsNumber ()) {
		ThrowException (Exception::TypeError (String::New (
				"Option argument must be a number")));
		return scope.Close (args.This ());
	}

	int level = args[0]->ToInt32 ()->Value ();
	int option = args[1]->ToInt32 ()->Value ();
	SOCKET_OPT_TYPE val = NULL;
	unsigned int ival = 0;
	SOCKET_LEN_TYPE len;

	if (args.Length () > 3) {
		if (! node::Buffer::HasInstance (args[2])) {
			ThrowException (Exception::TypeError (String::New (
					"Value argument must be a node Buffer object if length is "
					"provided")));
			return scope.Close (args.This ());
		}
		
		Local<Object> buffer = args[2]->ToObject ();
		val = node::Buffer::Data (buffer);

		if (! args[3]->IsInt32 ()) {
			ThrowException (Exception::TypeError (String::New (
					"Length argument must be an unsigned integer")));
			return scope.Close (args.This ());
		}

		len = args[3]->ToInt32 ()->Value ();

		if (len > node::Buffer::Length (buffer)) {
			ThrowException (Exception::TypeError (String::New (
					"Length argument is larger than buffer length")));
			return scope.Close (args.This ());
		}
	} else {
		if (! args[2]->IsUint32 ()) {
			ThrowException (Exception::TypeError (String::New (
					"Value argument must be a unsigned integer")));
			return scope.Close (args.This ());
		}

		ival = args[2]->ToUint32 ()->Value ();
		len = 4;
	}

	int rc = setsockopt (socket->poll_fd_, level, option,
			(val ? val : (SOCKET_OPT_TYPE) &ival), len);

	if (rc == SOCKET_ERROR) {
		ThrowException (Exception::Error (String::New (
				raw_strerror (SOCKET_ERRNO))));
		return scope.Close (args.This ());
	}
	
	return scope.Close (args.This ());
}

static void IoEvent (uv_poll_t* watcher, int status, int revents) {
	SocketWrap *socket = static_cast<SocketWrap*>(watcher->data);
	socket->HandleIOEvent (status, revents);
}

}; /* namespace raw */

#endif /* RAW_CC */
