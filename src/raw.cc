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

	SocketWrap::Init (target);
}

NODE_MODULE(raw, InitAll)

void SocketWrap::Init (Handle<Object> target) {
	HandleScope scope;
	
	Local<FunctionTemplate> tpl = FunctionTemplate::New (New);
	
	tpl->InstanceTemplate ()->SetInternalFieldCount (1);
	tpl->SetClassName (String::NewSymbol ("SocketWrap"));
	
	NODE_SET_PROTOTYPE_METHOD(tpl, "close", Close);
	NODE_SET_PROTOTYPE_METHOD(tpl, "generateChecksums", GenerateChecksums);
	NODE_SET_PROTOTYPE_METHOD(tpl, "noIpHeader", NoIpHeader);
	NODE_SET_PROTOTYPE_METHOD(tpl, "recv", Recv);
	NODE_SET_PROTOTYPE_METHOD(tpl, "send", Send);
	NODE_SET_PROTOTYPE_METHOD(tpl, "pause", Pause);
	
	target->Set (String::NewSymbol ("SocketWrap"), tpl->GetFunction ());
}

SocketWrap::SocketWrap () {}

SocketWrap::~SocketWrap () {
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
		uv_poll_stop (&this->poll_watcher_);
		closesocket (this->poll_fd_);
		this->poll_fd_ = INVALID_SOCKET;

		uv_close ((uv_handle_t *) &this->poll_watcher_, OnClose);
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

	uv_poll_init_socket (uv_default_loop (), &this->poll_watcher_,
			this->poll_fd_);
	this->poll_watcher_.data = this;
	uv_poll_start (&this->poll_watcher_, UV_READABLE, IoEvent);
	
	this->poll_initialised_ = true;
	
	return 0;
}

Handle<Value> SocketWrap::GenerateChecksums (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	
	if (args.Length () < 1) {
		ThrowException (Exception::Error (String::New (
				"At least one argument is required")));
		return scope.Close (args.This ());
	}
	
	if (! args[0]->IsBoolean ()) {
		ThrowException (Exception::TypeError (String::New (
				"Generate argument must be a boolean")));
		return scope.Close (args.This ());
	} else {
		socket->generate_checksums_ = args[0]->ToBoolean ()->Value ();
	}
	
	if (args.Length () > 1) {
		if (! args[1]->IsUint32 ()) {
			ThrowException (Exception::TypeError (String::New (
					"Offset argument must be an unsigned integer")));
			return scope.Close (args.This ());
		} else {
			socket->checksum_offset_ = args[1]->ToUint32 ()->Value ();
		}
	} else {
		socket->checksum_offset_ = 0;
	}

	return scope.Close (args.This ());
}

Handle<Value> SocketWrap::NoIpHeader (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	int rc;
	
	if (args.Length () < 1) {
		ThrowException (Exception::Error (String::New (
				"At least one argument is required")));
		return scope.Close (args.This ());
	}
	
	if (! args[0]->IsBoolean ()) {
		ThrowException (Exception::TypeError (String::New (
				"noHeader argument must be a boolean")));
		return scope.Close (args.This ());
	} else {
		socket->no_ip_header_ = args[0]->ToBoolean ()->Value ();
	}

#ifdef _WIN32
	DWORD val = socket->no_ip_header_ ? FALSE : FALSE;
	if (socket->family_ == AF_INET6)
		rc = setsockopt (socket->poll_fd_, IPPROTO_IPV6, IPV6_HDRINCL,
				(const char *) &val, sizeof (val));
	else
		rc = setsockopt (socket->poll_fd_, IPPROTO_IP, IP_HDRINCL,
				(const char *) &val, sizeof (val));
#else
	int val = socket->no_ip_header_ ? 1 : 0;
	if (socket->family_ == AF_INET6) {
		ThrowException (Exception::Error (String::New ("The noIpHeader option "
				"is not supported for IPv6 on this platform")));
		return scope.Close (args.This ());
	} else {	
		rc = setsockopt (socket->poll_fd_, IPPROTO_IP, IP_HDRINCL,
				(void *) &val, sizeof (val));
	}
#endif

	if (rc == SOCKET_ERROR) {
		ThrowException (Exception::Error (String::New (
				raw_strerror (SOCKET_ERRNO))));
		return scope.Close (args.This ());
	}

	return scope.Close (args.This ());
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

void SocketWrap::OnClose (uv_handle_t *handle) {
	// We can re-use the socket so we won't actually do anything here
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
	
	socket->generate_checksums_ = false;
	
	socket->no_ip_header_ = false;

	rc = socket->CreateSocket ();
	if (rc != 0) {
		ThrowException (Exception::Error (String::New (raw_strerror (rc))));
		return scope.Close (args.This ());
	}

	socket->Wrap (args.This ());

	return scope.Close (args.This ());
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

	if (! args[0]->IsBoolean ()) {
		ThrowException (Exception::TypeError (String::New (
				"Send argument must be a boolean")));
		return scope.Close (args.This ());
	}
	bool pause_send = args[0]->ToBoolean ()->Value ();
	
	int events = (pause_recv ? 0 : UV_READABLE)
			| (pause_send ? 0 : UV_WRITABLE);

	uv_poll_stop (&socket->poll_watcher_);
	uv_poll_start (&socket->poll_watcher_, events, IoEvent);
	
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

static uint32_t checksum (unsigned char *buffer, unsigned length) {
	unsigned i;
	uint32_t sum = 0;

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

Handle<Value> SocketWrap::Send (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	Local<Object> buffer;
	uint32_t offset;
	uint32_t length;
	int rc;
	unsigned int sum;
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
	
	if (socket->generate_checksums_) {
		if (socket->checksum_offset_ + 2 > length) {
			ThrowException (Exception::Error (String::New ("Checksum offset "
					"is out of bounds")));
			return scope.Close (args.This ());
		} else {
			*(data + socket->checksum_offset_) = 0;
			*(data + socket->checksum_offset_ + 1) = 0;
			sum = checksum ((unsigned char *) data + offset, length);
			*(data + socket->checksum_offset_) = (sum >> 8) & 0xff;
			*(data + socket->checksum_offset_ + 1) = sum & 0xff;
		}
	}
	
	if (socket->family_ == AF_INET6) {
		struct sockaddr_in6 addr = uv_ip6_addr (*address, 0);
		rc = sendto (socket->poll_fd_, data, length, 0,
				(struct sockaddr *) &addr, sizeof (addr));
	} else {
		struct sockaddr_in addr = uv_ip4_addr (*address, 0);
		rc = sendto (socket->poll_fd_, data, length, 0,
				(struct sockaddr *) &addr, sizeof (addr));
	}
	
	// Zero out the checksum again, in case it needs to be re-used...
	if (socket->generate_checksums_) {
		*(data + socket->checksum_offset_) = 0;
		*(data + socket->checksum_offset_ + 1) = 0;
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

static void IoEvent (uv_poll_t* watcher, int status, int revents) {
	SocketWrap *socket = static_cast<SocketWrap*>(watcher->data);
	socket->HandleIOEvent (status, revents);
}

}; /* namespace raw */

#endif /* RAW_CC */
