#ifndef RAW_CC
#define RAW_CC

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

static Nan::Persistent<FunctionTemplate> SocketWrap_constructor;

void InitAll (Handle<Object> exports) {
	ExportConstants (exports);
	ExportFunctions (exports);

	SocketWrap::Init (exports);
}

NODE_MODULE(raw, InitAll)

NAN_METHOD(CreateChecksum) {
	Nan::HandleScope scope;
	
	if (info.Length () < 2) {
		Nan::ThrowError("At least one argument is required");
		return;
	}

	if (! info[0]->IsUint32 ()) {
		Nan::ThrowTypeError("Start with argument must be an unsigned integer");
		return;
	}
	
	uint32_t start_with = info[0]->ToUint32 ()->Value ();

	if (start_with > 65535) {
		Nan::ThrowRangeError("Start with argument cannot be larger than 65535");
		return;
	}

	if (! node::Buffer::HasInstance (info[1])) {
		Nan::ThrowTypeError("Buffer argument must be a node Buffer object");
		return;
	}
	
	Local<Object> buffer = info[1]->ToObject ();
	char *data = node::Buffer::Data (buffer);
	size_t length = node::Buffer::Length (buffer);
	unsigned int offset = 0;
	
	if (info.Length () > 2) {
		if (! info[2]->IsUint32 ()) {
			Nan::ThrowTypeError("Offset argument must be an unsigned integer");
			return;
		}
		offset = info[2]->ToUint32 ()->Value ();
		if (offset >= length) {
			Nan::ThrowRangeError("Offset argument must be smaller than length of the buffer");
			return;
		}
	}
	
	if (info.Length () > 3) {
		if (! info[3]->IsUint32 ()) {
			Nan::ThrowTypeError("Length argument must be an unsigned integer");
			return;
		}
		unsigned int new_length = info[3]->ToUint32 ()->Value ();
		if (new_length > length) {
			Nan::ThrowRangeError("Length argument must be smaller than length of the buffer");
			return;
		}
		length = new_length;
	}
	
	uint16_t sum = checksum ((uint16_t) start_with,
			(unsigned char *) data + offset, length);

	Local<Integer> number = Nan::New<Uint32>(sum);
	
	info.GetReturnValue().Set(number);
}

NAN_METHOD(Htonl) {
	Nan::HandleScope scope;

	if (info.Length () < 1) {
		Nan::ThrowError("One arguments is required");
		return;
	}

	if (! info[0]->IsUint32 ()) {
		Nan::ThrowTypeError("Number must be a 32 unsigned integer");
		return;
	}

	unsigned int number = info[0]->ToUint32 ()->Value ();
	Local<Uint32> converted = Nan::New<Uint32>((unsigned int) htonl (number));

	info.GetReturnValue().Set(converted);
}

NAN_METHOD(Htons) {
	Nan::HandleScope scope;
	
	if (info.Length () < 1) {
		Nan::ThrowError("One arguments is required");
		return;
	}

	if (! info[0]->IsUint32 ()) {
		Nan::ThrowTypeError("Number must be a 16 unsigned integer");
		return;
	}
	
	unsigned int number = info[0]->ToUint32 ()->Value ();
	
	if (number > 65535) {
		Nan::ThrowRangeError("Number cannot be larger than 65535");
		return;
	}
	
	Local<Uint32> converted = Nan::New<Uint32>(htons (number));

	info.GetReturnValue().Set(converted);
}

NAN_METHOD(Ntohl) {
	Nan::HandleScope scope;
	
	if (info.Length () < 1) {
		Nan::ThrowError("One arguments is required");
		return;
	}

	if (! info[0]->IsUint32 ()) {
		Nan::ThrowTypeError("Number must be a 32 unsigned integer");
		return;
	}

	unsigned int number = info[0]->ToUint32 ()->Value ();
	Local<Uint32> converted = Nan::New<Uint32>((unsigned int) ntohl (number));

	info.GetReturnValue().Set(converted);
}

NAN_METHOD(Ntohs) {
	Nan::HandleScope scope;
	
	if (info.Length () < 1) {
		Nan::ThrowError("One arguments is required");
		return;
	}

	if (! info[0]->IsUint32 ()) {
		Nan::ThrowTypeError("Number must be a 16 unsigned integer");
		return;
	}
	
	unsigned int number = info[0]->ToUint32 ()->Value ();
	
	if (number > 65535) {
		Nan::ThrowRangeError("Number cannot be larger than 65535");
		return;
	}
	
	Local<Uint32> converted = Nan::New<Uint32>(htons (number));

	info.GetReturnValue().Set(converted);
}

void ExportConstants (Handle<Object> target) {
	Local<Object> socket_level = Nan::New<Object>();
	Local<Object> socket_option = Nan::New<Object>();

	Nan::Set(target, Nan::New("SocketLevel").ToLocalChecked(), socket_level);
	Nan::Set(target, Nan::New("SocketOption").ToLocalChecked(), socket_option);

	Nan::Set(socket_level, Nan::New("SOL_SOCKET").ToLocalChecked(), Nan::New<Number>(SOL_SOCKET));
	Nan::Set(socket_level, Nan::New("IPPROTO_IP").ToLocalChecked(), Nan::New<Number>(IPPROTO_IP + 0));
	Nan::Set(socket_level, Nan::New("IPPROTO_IPV6").ToLocalChecked(), Nan::New<Number>(IPPROTO_IPV6 + 0));

	Nan::Set(socket_option, Nan::New("SO_BROADCAST").ToLocalChecked(), Nan::New<Number>(SO_BROADCAST));
	Nan::Set(socket_option, Nan::New("SO_RCVBUF").ToLocalChecked(), Nan::New<Number>(SO_RCVBUF));
	Nan::Set(socket_option, Nan::New("SO_RCVTIMEO").ToLocalChecked(), Nan::New<Number>(SO_RCVTIMEO));
	Nan::Set(socket_option, Nan::New("SO_SNDBUF").ToLocalChecked(), Nan::New<Number>(SO_SNDBUF));
	Nan::Set(socket_option, Nan::New("SO_SNDTIMEO").ToLocalChecked(), Nan::New<Number>(SO_SNDTIMEO));

#ifdef __linux__
	Nan::Set(socket_option, Nan::New("SO_BINDTODEVICE").ToLocalChecked(), Nan::New<Number>(SO_BINDTODEVICE));
#endif

	Nan::Set(socket_option, Nan::New("IP_HDRINCL").ToLocalChecked(), Nan::New<Number>(IP_HDRINCL));
	Nan::Set(socket_option, Nan::New("IP_OPTIONS").ToLocalChecked(), Nan::New<Number>(IP_OPTIONS));
	Nan::Set(socket_option, Nan::New("IP_TOS").ToLocalChecked(), Nan::New<Number>(IP_TOS));
	Nan::Set(socket_option, Nan::New("IP_TTL").ToLocalChecked(), Nan::New<Number>(IP_TTL));

#ifdef _WIN32
	Nan::Set(socket_option, Nan::New("IPV6_HDRINCL").ToLocalChecked(), Nan::New<Number>(IPV6_HDRINCL));
#endif
	Nan::Set(socket_option, Nan::New("IPV6_TTL").ToLocalChecked(), Nan::New<Number>(IPV6_UNICAST_HOPS));
	Nan::Set(socket_option, Nan::New("IPV6_UNICAST_HOPS").ToLocalChecked(), Nan::New<Number>(IPV6_UNICAST_HOPS));
	Nan::Set(socket_option, Nan::New("IPV6_V6ONLY").ToLocalChecked(), Nan::New<Number>(IPV6_V6ONLY));
}

void ExportFunctions (Handle<Object> target) {
	Nan::Set(target, Nan::New("createChecksum").ToLocalChecked(), Nan::New<FunctionTemplate>(CreateChecksum)->GetFunction ());
	
	Nan::Set(target, Nan::New("htonl").ToLocalChecked(), Nan::New<FunctionTemplate>(Htonl)->GetFunction ());
	Nan::Set(target, Nan::New("htons").ToLocalChecked(), Nan::New<FunctionTemplate>(Htons)->GetFunction ());
	Nan::Set(target, Nan::New("ntohl").ToLocalChecked(), Nan::New<FunctionTemplate>(Ntohl)->GetFunction ());
	Nan::Set(target, Nan::New("ntohs").ToLocalChecked(), Nan::New<FunctionTemplate>(Ntohs)->GetFunction ());
}

void SocketWrap::Init (Handle<Object> exports) {
	Nan::HandleScope scope;

	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(SocketWrap::New);
	tpl->SetClassName(Nan::New("SocketWrap").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	Nan::SetPrototypeMethod(tpl, "close", Close);
	Nan::SetPrototypeMethod(tpl, "getOption", GetOption);
	Nan::SetPrototypeMethod(tpl, "pause", Pause);
	Nan::SetPrototypeMethod(tpl, "recv", Recv);
	Nan::SetPrototypeMethod(tpl, "send", Send);
	Nan::SetPrototypeMethod(tpl, "setOption", SetOption);

	SocketWrap_constructor.Reset(tpl);
	exports->Set(Nan::New("SocketWrap").ToLocalChecked(),
			Nan::GetFunction(tpl).ToLocalChecked());
}

SocketWrap::SocketWrap () {
	deconstructing_ = false;
}

SocketWrap::~SocketWrap () {
	deconstructing_ = true;
	this->CloseSocket ();
}

NAN_METHOD(SocketWrap::Close) {
	Nan::HandleScope scope;
	
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (info.This ());
	
	socket->CloseSocket ();

	info.GetReturnValue().Set(info.This());
}

void SocketWrap::CloseSocket (void) {
	Nan::HandleScope scope;
	
	if (this->poll_initialised_) {
		uv_close ((uv_handle_t *) this->poll_watcher_, OnClose);
		closesocket (this->poll_fd_);
		this->poll_fd_ = INVALID_SOCKET;
		this->poll_initialised_ = false;
	}

	if (! this->deconstructing_) {
		Local<Value> emit = handle()->Get(Nan::New<String>("emit").ToLocalChecked());
		Local<Function> cb = emit.As<Function> ();

		Local<Value> args[1];
		args[0] = Nan::New<String>("close").ToLocalChecked();

		cb->Call (handle(), 1, args);
	}
}

int SocketWrap::CreateSocket (void) {
	if (this->poll_initialised_)
		return 0;
	
	this->poll_fd_ = socket (this->family_, SOCK_RAW, this->protocol_);
	
#ifdef __APPLE__
	/**
	 ** On MAC OS X platforms for non-privileged users wishing to utilise ICMP
	 ** a SOCK_DGRAM will be enough, so try to create this type of socket in
	 ** the case ICMP was requested.
	 **
	 ** More information can be found at:
	 **
	 **  https://developer.apple.com/library/mac/documentation/Darwin/Reference/Manpages/man4/icmp.4.html
	 **
	 **/
	if (this->poll_fd_ == INVALID_SOCKET && this->protocol_ == IPPROTO_ICMP)
		this->poll_fd_ = socket (this->family_, SOCK_DGRAM, this->protocol_);
#endif

	if (this->poll_fd_ == INVALID_SOCKET)
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

NAN_METHOD(SocketWrap::GetOption) {
	Nan::HandleScope scope;
	
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (info.This ());
	
	if (info.Length () < 3) {
		Nan::ThrowError("Three arguments are required");
		return;
	}

	if (! info[0]->IsNumber ()) {
		Nan::ThrowTypeError("Level argument must be a number");
		return;
	}

	if (! info[1]->IsNumber ()) {
		Nan::ThrowTypeError("Option argument must be a number");
		return;
	}

	int level = info[0]->ToInt32 ()->Value ();
	int option = info[1]->ToInt32 ()->Value ();
	SOCKET_OPT_TYPE val = NULL;
	unsigned int ival = 0;
	SOCKET_LEN_TYPE len;

	if (! node::Buffer::HasInstance (info[2])) {
		Nan::ThrowTypeError("Value argument must be a node Buffer object if length is provided");
		return;
	}
	
	Local<Object> buffer = info[2]->ToObject ();
	val = node::Buffer::Data (buffer);

	if (! info[3]->IsInt32 ()) {
		Nan::ThrowTypeError("Length argument must be an unsigned integer");
		return;
	}

	len = (SOCKET_LEN_TYPE) node::Buffer::Length (buffer);

	int rc = getsockopt (socket->poll_fd_, level, option,
			(val ? val : (SOCKET_OPT_TYPE) &ival), &len);

	if (rc == SOCKET_ERROR) {
		Nan::ThrowError(raw_strerror (SOCKET_ERRNO));
		return;
	}
	
	Local<Number> got = Nan::New<Uint32>(len);
	
	info.GetReturnValue().Set(got);
}

void SocketWrap::HandleIOEvent (int status, int revents) {
	Nan::HandleScope scope;

	if (status) {
		Local<Value> emit = handle()->Get (Nan::New<String>("emit").ToLocalChecked());
		Local<Function> cb = emit.As<Function> ();

		Local<Value> args[2];
		args[0] = Nan::New<String>("error").ToLocalChecked();
		
		/**
		 ** The uv_last_error() function doesn't seem to be available in recent
		 ** libuv versions, and the uv_err_t variable also no longer appears to
		 ** be a structure.  This causes issues when working with both Node.js
		 ** 0.10 and 0.12.  So, for now, we will just give you the number.
		 **/
		char status_str[32];
		sprintf(status_str, "%d", status);
		args[1] = Nan::Error(status_str);

		cb->Call (handle(), 2, args);
	} else {
		Local<Value> emit = handle()->Get (Nan::New<String>("emit").ToLocalChecked());
		Local<Function> cb = emit.As<Function> ();

		Local<Value> args[1];
		if (revents & UV_READABLE)
			args[0] = Nan::New<String>("recvReady").ToLocalChecked();
		else
			args[0] = Nan::New<String>("sendReady").ToLocalChecked();

		cb->Call (handle(), 1, args);
	}
}

NAN_METHOD(SocketWrap::New) {
	Nan::HandleScope scope;
	
	SocketWrap* socket = new SocketWrap ();
	int rc, family = AF_INET;
	
	if (info.Length () < 1) {
		Nan::ThrowError("One argument is required");
		return;
	}
	
	if (! info[0]->IsUint32 ()) {
		Nan::ThrowTypeError("Protocol argument must be an unsigned integer");
		return;
	} else {
		socket->protocol_ = info[0]->ToUint32 ()->Value ();
	}

	if (info.Length () > 1) {
		if (! info[1]->IsUint32 ()) {
			Nan::ThrowTypeError("Address family argument must be an unsigned integer");
			return;
		} else {
			if (info[1]->ToUint32 ()->Value () == 2)
				family = AF_INET6;
		}
	}
	
	socket->family_ = family;
	
	socket->poll_initialised_ = false;
	
	socket->no_ip_header_ = false;

	rc = socket->CreateSocket ();
	if (rc != 0) {
		Nan::ThrowError(raw_strerror (rc));
		return;
	}

	socket->Wrap (info.This ());

	info.GetReturnValue().Set(info.This());
}

void SocketWrap::OnClose (uv_handle_t *handle) {
	delete handle;
}

NAN_METHOD(SocketWrap::Pause) {
	Nan::HandleScope scope;
	
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (info.This ());

	if (info.Length () < 2) {
		Nan::ThrowError("Two arguments are required");
		return;
	}
	
	if (! info[0]->IsBoolean ()) {
		Nan::ThrowTypeError("Recv argument must be a boolean");
		return;
	}
	bool pause_recv = info[0]->ToBoolean ()->Value ();

	if (! info[1]->IsBoolean ()) {
		Nan::ThrowTypeError("Send argument must be a boolean");
		return;
	}
	bool pause_send = info[1]->ToBoolean ()->Value ();
	
	int events = (pause_recv ? 0 : UV_READABLE)
			| (pause_send ? 0 : UV_WRITABLE);

	if (! socket->deconstructing_) {
		uv_poll_stop (socket->poll_watcher_);
		if (events)
			uv_poll_start (socket->poll_watcher_, events, IoEvent);
	}
	
	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(SocketWrap::Recv) {
	Nan::HandleScope scope;
	
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (info.This ());
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
	
	if (info.Length () < 2) {
		Nan::ThrowError("Five arguments are required");
		return;
	}
	
	if (! node::Buffer::HasInstance (info[0])) {
		Nan::ThrowTypeError("Buffer argument must be a node Buffer object");
		return;
	} else {
		buffer = info[0]->ToObject ();
	}

	if (! info[1]->IsFunction ()) {
		Nan::ThrowTypeError("Callback argument must be a function");
		return;
	}

	rc = socket->CreateSocket ();
	if (rc != 0) {
		Nan::ThrowError(raw_strerror (errno));
		return;
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
		Nan::ThrowError(raw_strerror (SOCKET_ERRNO));
		return;
	}
	
	if (socket->family_ == AF_INET6)
		uv_ip6_name (&sin6_address, addr, 50);
	else
		uv_ip4_name (&sin_address, addr, 50);
	
	Local<Function> cb = Local<Function>::Cast (info[1]);
	const unsigned argc = 3;
	Local<Value> argv[argc];
	argv[0] = info[0];
	argv[1] = Nan::New<Number>(rc);
	argv[2] = Nan::New(addr).ToLocalChecked();
	cb->Call (socket->handle(), argc, argv);
	
	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(SocketWrap::Send) {
	Nan::HandleScope scope;
	
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (info.This ());
	Local<Object> buffer;
	uint32_t offset;
	uint32_t length;
	int rc;
	char *data;
	
	if (info.Length () < 5) {
		Nan::ThrowError("Five arguments are required");
		return;
	}
	
	if (! node::Buffer::HasInstance (info[0])) {
		Nan::ThrowTypeError("Buffer argument must be a node Buffer object");
		return;
	}
	
	if (! info[1]->IsUint32 ()) {
		Nan::ThrowTypeError("Offset argument must be an unsigned integer");
		return;
	}

	if (! info[2]->IsUint32 ()) {
		Nan::ThrowTypeError("Length argument must be an unsigned integer");
		return;
	}

	if (! info[3]->IsString ()) {
		Nan::ThrowTypeError("Address argument must be a string");
		return;
	}

	if (! info[4]->IsFunction ()) {
		Nan::ThrowTypeError("Callback argument must be a function");
		return;
	}

	rc = socket->CreateSocket ();
	if (rc != 0) {
		Nan::ThrowError(raw_strerror (errno));
		return;
	}
	
	buffer = info[0]->ToObject ();
	offset = info[1]->ToUint32 ()->Value ();
	length = info[2]->ToUint32 ()->Value ();

	data = node::Buffer::Data (buffer) + offset;
	
	if (socket->family_ == AF_INET6) {
#if UV_VERSION_MAJOR > 0
		struct sockaddr_in6 addr;

		uv_ip6_addr(*Nan::Utf8String(info[3]), 0, &addr);
#else
		String::Utf8String address (args[3]);
		struct sockaddr_in6 addr = uv_ip6_addr (*address, 0);
#endif
		
		rc = sendto (socket->poll_fd_, data, length, 0,
				(struct sockaddr *) &addr, sizeof (addr));
	} else {
#if UV_VERSION_MAJOR > 0
		struct sockaddr_in addr;
		uv_ip4_addr(*Nan::Utf8String(info[3]), 0, &addr);
#else
		String::Utf8String address (info[3]);
		struct sockaddr_in addr = uv_ip4_addr (*address, 0);
#endif

		rc = sendto (socket->poll_fd_, data, length, 0,
				(struct sockaddr *) &addr, sizeof (addr));
	}
	
	if (rc == SOCKET_ERROR) {
		Nan::ThrowError(raw_strerror (SOCKET_ERRNO));
		return;
	}
	
	Local<Function> cb = Local<Function>::Cast (info[4]);
	const unsigned argc = 1;
	Local<Value> argv[argc];
	argv[0] = Nan::New<Number>(rc);
	cb->Call (socket->handle(), argc, argv);
	
	info.GetReturnValue().Set(info.This());
}

NAN_METHOD(SocketWrap::SetOption) {
	Nan::HandleScope scope;
	
	SocketWrap* socket = SocketWrap::Unwrap<SocketWrap> (info.This ());
	
	if (info.Length () < 3) {
		Nan::ThrowError("Three or four arguments are required");
		return;
	}

	if (! info[0]->IsNumber ()) {
		Nan::ThrowTypeError("Level argument must be a number");
		return;
	}

	if (! info[1]->IsNumber ()) {
		Nan::ThrowTypeError("Option argument must be a number");
		return;
	}

	int level = info[0]->ToInt32 ()->Value ();
	int option = info[1]->ToInt32 ()->Value ();
	SOCKET_OPT_TYPE val = NULL;
	unsigned int ival = 0;
	SOCKET_LEN_TYPE len;

	if (info.Length () > 3) {
		if (! node::Buffer::HasInstance (info[2])) {
			Nan::ThrowTypeError("Value argument must be a node Buffer object if length is provided");
			return;
		}
		
		Local<Object> buffer = info[2]->ToObject ();
		val = node::Buffer::Data (buffer);

		if (! info[3]->IsInt32 ()) {
			Nan::ThrowTypeError("Length argument must be an unsigned integer");
			return;
		}

		len = info[3]->ToInt32 ()->Value ();

		if (len > node::Buffer::Length (buffer)) {
			Nan::ThrowTypeError("Length argument is larger than buffer length");
			return;
		}
	} else {
		if (! info[2]->IsUint32 ()) {
			Nan::ThrowTypeError("Value argument must be a unsigned integer");
			return;
		}

		ival = info[2]->ToUint32 ()->Value ();
		len = 4;
	}

	int rc = setsockopt (socket->poll_fd_, level, option,
			(val ? val : (SOCKET_OPT_TYPE) &ival), len);

	if (rc == SOCKET_ERROR) {
		Nan::ThrowError(raw_strerror(SOCKET_ERRNO));
		return;
	}
	
	info.GetReturnValue().Set(info.This());
}

static void IoEvent (uv_poll_t* watcher, int status, int revents) {
	SocketWrap *socket = static_cast<SocketWrap*>(watcher->data);
	socket->HandleIOEvent (status, revents);
}

}; /* namespace raw */

#endif /* RAW_CC */
