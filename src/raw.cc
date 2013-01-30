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
	NODE_SET_PROTOTYPE_METHOD(tpl, "recv", Recv);
	NODE_SET_PROTOTYPE_METHOD(tpl, "send", Send);
	NODE_SET_PROTOTYPE_METHOD(tpl, "startSendReady", StartSendReady);
	NODE_SET_PROTOTYPE_METHOD(tpl, "stopSendReady", StopSendReady);
	
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
	
	uv_poll_stop (&this->poll_watcher_);
	closesocket (this->poll_fd_);
	this->poll_fd_ = INVALID_SOCKET;

	Local<Value> emit = this->handle_->Get (EmitSymbol);
	Local<Function> cb = emit.As<Function> ();

	Local<Value> args[1];
	args[0] = Local<Value>::New (CloseSymbol);

	cb->Call (this->handle_, 1, args);
}

int SocketWrap::CreateSocket (void) {
	if (this->poll_initialised_)
		return 0;
	
	if ((this->poll_fd_ = socket (AF_INET, SOCK_RAW, this->protocol_))
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
	int rc;
	
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
	
	socket->poll_initialised_ = false;

	rc = socket->CreateSocket ();
	if (rc != 0) {
		ThrowException (Exception::Error (String::New (raw_strerror (rc))));
		return scope.Close (args.This ());
	}

	socket->Wrap (args.This ());

	return scope.Close (args.This ());
}

Handle<Value> SocketWrap::Recv (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	Local<Object> buffer;
	sockaddr_in sin_address;
	int rc;
#ifdef _WIN32
	int sin_length = sizeof (sin_address);
#else
	socklen_t sin_length = sizeof (sin_address);
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

	memset (&sin_address, 0, sizeof (sin_address));
	
	rc = recvfrom (socket->poll_fd_, node::Buffer::Data (buffer),
			(int) node::Buffer::Length (buffer), 0, (sockaddr *) &sin_address,
			&sin_length);
	if (rc == SOCKET_ERROR) {
		ThrowException (Exception::Error (String::New (raw_strerror (
				SOCKET_ERRNO))));
		return scope.Close (args.This ());
	}
	
	Local<Function> cb = Local<Function>::Cast (args[1]);
	const unsigned argc = 3;
	Local<Value> argv[argc];
	argv[0] = args[0];
	argv[1] = Number::New (rc);
	argv[2] = String::New (inet_ntoa (sin_address.sin_addr));
	cb->Call (Context::GetCurrent ()->Global (), argc, argv);
	
	return scope.Close (args.This ());
}

Handle<Value> SocketWrap::Send (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	Local<Object> buffer;
	uint32_t offset;
	uint32_t length;
	sockaddr_in sin_address;
	int rc;
	
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

	memset (&sin_address, 0, sizeof (sin_address));
	sin_address.sin_family = AF_INET;
	sin_address.sin_port = htons (0);
	sin_address.sin_addr.s_addr = inet_addr (*address);
	
	rc = sendto (socket->poll_fd_, node::Buffer::Data (buffer) + offset, length,
			0, (sockaddr *) &sin_address, sizeof (sin_address));
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

Handle<Value> SocketWrap::StartSendReady (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	
	uv_poll_stop (&socket->poll_watcher_);
	uv_poll_start (&socket->poll_watcher_, UV_READABLE | UV_WRITABLE, IoEvent);
	
	return scope.Close (args.This ());
}

Handle<Value> SocketWrap::StopSendReady (const Arguments& args) {
	HandleScope scope;
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (args.This ());
	
	uv_poll_stop (&socket->poll_watcher_);
	uv_poll_start (&socket->poll_watcher_, UV_READABLE, IoEvent);
	
	return scope.Close (args.This ());
}

static void IoEvent (uv_poll_t* watcher, int status, int revents) {
	SocketWrap *socket = static_cast<SocketWrap*>(watcher->data);
	socket->HandleIOEvent (status, revents);
}

}; /* namespace raw */

#endif /* RAW_CC */
