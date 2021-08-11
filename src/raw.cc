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

static Napi::FunctionReference SocketWrap_constructor;

Napi::Object InitAll (Napi::Env env, Napi::Object exports) {
	ExportConstants (env, exports);
	ExportFunctions (env, exports);

	SocketWrap::Init (env, exports);

	return exports;
}

NODE_API_MODULE(raw, InitAll)

Napi::Value CreateChecksum(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	if (info.Length () < 2) {
		Napi::Error::New(env, "At least one argument is required").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[0].IsNumber ()) {
		Napi::TypeError::New(env, "Start with argument must be an unsigned integer").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	uint32_t start_with = info[0].As<Napi::Number>();

	if (start_with > 65535) {
		Napi::RangeError::New(env, "Start with argument cannot be larger than 65535").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[1].IsBuffer()) {
		Napi::TypeError::New(env, "Buffer argument must be a node Buffer object").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	Napi::Buffer<char> buffer = info[1].As<Napi::Buffer<char> >();
	char *data = buffer.Data();
	size_t length = buffer.Length();
	unsigned int offset = 0;
	
	if (info.Length () > 2) {
		if (! info[2].IsNumber ()) {
			Napi::TypeError::New(env, "Offset argument must be an unsigned integer").ThrowAsJavaScriptException();

			return env.Undefined();
		}
		offset = info[2].As<Napi::Number>();
		if (offset >= length) {
			Napi::RangeError::New(env, "Offset argument must be smaller than length of the buffer").ThrowAsJavaScriptException();

			return env.Undefined();
		}
	}
	
	if (info.Length () > 3) {
		if (! info[3].IsNumber ()) {
			Napi::TypeError::New(env, "Length argument must be an unsigned integer").ThrowAsJavaScriptException();

			return env.Undefined();
		}
		unsigned int new_length = info[3].As<Napi::Number>();
		if (new_length > length) {
			Napi::RangeError::New(env, "Length argument must be smaller than length of the buffer").ThrowAsJavaScriptException();

			return env.Undefined();
		}
		length = new_length;
	}
	
	uint16_t sum = checksum ((uint16_t) start_with,
			(unsigned char *) data + offset, length);

	Napi::Number number = Napi::Number::New(env, sum);
	return number;
}

Napi::Value Htonl(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);

	if (info.Length () < 1) {
		Napi::Error::New(env, "One arguments is required").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[0].IsNumber ()) {
		Napi::TypeError::New(env, "Number must be a 32 unsigned integer").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	unsigned int number = info[0].As<Napi::Number>();
	Napi::Number converted = Napi::Number::New(env, (unsigned int) htonl (number));

	return converted;
}

Napi::Value Htons(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	if (info.Length () < 1) {
		Napi::Error::New(env, "One arguments is required").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[0].IsNumber ()) {
		Napi::TypeError::New(env, "Number must be a 16 unsigned integer").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	unsigned int number = info[0].As<Napi::Number>();
	
	if (number > 65535) {
		Napi::RangeError::New(env, "Number cannot be larger than 65535").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	Napi::Number converted = Napi::Number::New(env, htons (number));

	return converted;
}

Napi::Value Ntohl(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	if (info.Length () < 1) {
		Napi::Error::New(env, "One arguments is required").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[0].IsNumber ()) {
		Napi::TypeError::New(env, "Number must be a 32 unsigned integer").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	unsigned int number = info[0].As<Napi::Number>();
	Napi::Number converted = Napi::Number::New(env, (unsigned int) ntohl (number));

	return converted;
}

Napi::Value Ntohs(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	if (info.Length () < 1) {
		Napi::Error::New(env, "One arguments is required").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[0].IsNumber ()) {
		Napi::TypeError::New(env, "Number must be a 16 unsigned integer").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	unsigned int number = info[0].As<Napi::Number>();
	
	if (number > 65535) {
		Napi::RangeError::New(env, "Number cannot be larger than 65535").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	Napi::Number converted = Napi::Number::New(env, ntohs (number));

	return converted;
}

void ExportConstants (Napi::Env env, Napi::Object target) {
	Napi::Object socket_level = Napi::Object::New(env);
	Napi::Object socket_option = Napi::Object::New(env);

	(target).Set(Napi::String::New(env, "SocketLevel"), socket_level);
	(target).Set(Napi::String::New(env, "SocketOption"), socket_option);

	(socket_level).Set(Napi::String::New(env, "SOL_SOCKET"), Napi::Number::New(env, SOL_SOCKET));
	(socket_level).Set(Napi::String::New(env, "IPPROTO_IP"), Napi::Number::New(env, IPPROTO_IP + 0));
	(socket_level).Set(Napi::String::New(env, "IPPROTO_IPV6"), Napi::Number::New(env, IPPROTO_IPV6 + 0));

	(socket_option).Set(Napi::String::New(env, "SO_BROADCAST"), Napi::Number::New(env, SO_BROADCAST));
	(socket_option).Set(Napi::String::New(env, "SO_RCVBUF"), Napi::Number::New(env, SO_RCVBUF));
	(socket_option).Set(Napi::String::New(env, "SO_RCVTIMEO"), Napi::Number::New(env, SO_RCVTIMEO));
	(socket_option).Set(Napi::String::New(env, "SO_SNDBUF"), Napi::Number::New(env, SO_SNDBUF));
	(socket_option).Set(Napi::String::New(env, "SO_SNDTIMEO"), Napi::Number::New(env, SO_SNDTIMEO));

#ifdef __linux__
	(socket_option).Set(Napi::String::New(env, "SO_BINDTODEVICE"), Napi::Number::New(env, SO_BINDTODEVICE));
#endif

	(socket_option).Set(Napi::String::New(env, "IP_HDRINCL"), Napi::Number::New(env, IP_HDRINCL));
	(socket_option).Set(Napi::String::New(env, "IP_OPTIONS"), Napi::Number::New(env, IP_OPTIONS));
	(socket_option).Set(Napi::String::New(env, "IP_TOS"), Napi::Number::New(env, IP_TOS));
	(socket_option).Set(Napi::String::New(env, "IP_TTL"), Napi::Number::New(env, IP_TTL));

#ifdef _WIN32
	(socket_option).Set(Napi::String::New(env, "IPV6_HDRINCL"), Napi::Number::New(env, IPV6_HDRINCL));
#endif
	(socket_option).Set(Napi::String::New(env, "IPV6_TTL"), Napi::Number::New(env, IPV6_UNICAST_HOPS));
	(socket_option).Set(Napi::String::New(env, "IPV6_UNICAST_HOPS"), Napi::Number::New(env, IPV6_UNICAST_HOPS));
	(socket_option).Set(Napi::String::New(env, "IPV6_V6ONLY"), Napi::Number::New(env, IPV6_V6ONLY));
}

void ExportFunctions (Napi::Env env, Napi::Object target) {
	(target).Set(Napi::String::New(env, "createChecksum"), Napi::Function::New(env, CreateChecksum));
	
	(target).Set(Napi::String::New(env, "htonl"), Napi::Function::New(env, Htonl));
	(target).Set(Napi::String::New(env, "htons"), Napi::Function::New(env, Htons));
	(target).Set(Napi::String::New(env, "ntohl"), Napi::Function::New(env, Ntohl));
	(target).Set(Napi::String::New(env, "ntohs"), Napi::Function::New(env, Ntohs));
}

void SocketWrap::Init (Napi::Env env, Napi::Object exports) {
	Napi::HandleScope scope(env);

	Napi::Function tpl = DefineClass(env, "SocketWrap", {
			InstanceMethod<&SocketWrap::Close>("close"),
			InstanceMethod<&SocketWrap::GetOption>("getOption"),
			InstanceMethod<&SocketWrap::Pause>("pause"),
			InstanceMethod<&SocketWrap::Recv>("recv"),
			InstanceMethod<&SocketWrap::Send>("send"),
			InstanceMethod<&SocketWrap::SetOption>("setOption")
	});
	SocketWrap_constructor = Napi::Persistent(tpl);
	SocketWrap_constructor.SuppressDestruct();

	(exports).Set(Napi::String::New(env, "SocketWrap"), tpl);
}

SocketWrap::~SocketWrap () {
	deconstructing_ = true;
	this->CloseSocket ();
}

Napi::Value SocketWrap::Close(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	SocketWrap* socket = this;
	
	socket->CloseSocket ();

	Napi::Value args[1];
	args[0] = Napi::String::New(env, "close");

	Napi::Value(info.This().As<Napi::Object>()["emit"]).As<Napi::Function>().Call(info.This(), std::initializer_list<napi_value>{ args[0] });

	return info.This();
}
void SocketWrap::CloseSocket (void) {
	if (this->poll_initialised_) {
		uv_close ((uv_handle_t *) this->poll_watcher_, OnClose);
		closesocket (this->poll_fd_);
		this->poll_fd_ = INVALID_SOCKET;
		this->poll_initialised_ = false;
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

Napi::Value SocketWrap::GetOption(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	SocketWrap* socket = this;
	
	if (info.Length () < 3) {
		Napi::Error::New(env, "Three arguments are required").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[0].IsNumber ()) {
		Napi::TypeError::New(env, "Level argument must be a number").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[1].IsNumber ()) {
		Napi::TypeError::New(env, "Option argument must be a number").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	int level = info[0].As<Napi::Number>();
	int option = info[1].As<Napi::Number>();
	SOCKET_OPT_TYPE val = NULL;
	unsigned int ival = 0;
	SOCKET_LEN_TYPE len;

	if (! info[2].IsBuffer()) {
		Napi::TypeError::New(env, "Value argument must be a node Buffer object if length is provided").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	Napi::Buffer<char> buffer = info[2].As<Napi::Buffer<char> >();
	val = buffer.Data();

	if (! info[3].IsNumber ()) {
		Napi::TypeError::New(env, "Length argument must be an unsigned integer").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	len = (SOCKET_LEN_TYPE) buffer.Length();

	int rc = getsockopt (socket->poll_fd_, level, option,
			(val ? val : (SOCKET_OPT_TYPE) &ival), &len);

	if (rc == SOCKET_ERROR) {
		Napi::Error::New(env, raw_strerror (SOCKET_ERRNO)).ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	Napi::Number got = Napi::Number::New(env, len);
	
	return got;
}

Napi::Value SocketWrap::handle() {
	return Value();
}

void SocketWrap::HandleIOEvent (int status, int revents) {
	Napi::Env env = Env();
	Napi::HandleScope scope(env);

	if (status) {
		Napi::Value args[2];
		args[0] = Napi::String::New(env, "error");
		
		/**
		 ** The uv_last_error() function doesn't seem to be available in recent
		 ** libuv versions, and the uv_err_t variable also no longer appears to
		 ** be a structure.  This causes issues when working with both Node.js
		 ** 0.10 and 0.12.  So, for now, we will just give you the number.
		 **/
		char status_str[32];
		sprintf(status_str, "%d", status);
		Napi::Error error = Napi::Error::New(env, status_str);
		args[1] = error.Value();

		Napi::Value(handle().As<Napi::Object>()["emit"]).As<Napi::Function>().Call(handle(), std::initializer_list<napi_value>{ args[0] });
	} else {
		Napi::Value args[1];
		if (revents & UV_READABLE)
			args[0] = Napi::String::New(env, "recvReady");
		else
			args[0] = Napi::String::New(env, "sendReady");

		Napi::Value(handle().As<Napi::Object>()["emit"]).As<Napi::Function>().Call(handle(), std::initializer_list<napi_value>{ args[0] });
	}
}

SocketWrap::SocketWrap(const Napi::CallbackInfo& info) : Napi::ObjectWrap<SocketWrap>(info), deconstructing_(false) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	SocketWrap* socket = this;
	int rc, family = AF_INET;
	
	if (info.Length () < 1) {
		Napi::Error::New(env, "One argument is required").ThrowAsJavaScriptException();

		return;
	}
	
	if (! info[0].IsNumber ()) {
		Napi::TypeError::New(env, "Protocol argument must be an unsigned integer").ThrowAsJavaScriptException();

		return;
	} else {
		socket->protocol_ = info[0].As<Napi::Number>();
	}

	if (info.Length () > 1) {
		if (! info[1].IsNumber ()) {
			Napi::TypeError::New(env, "Address family argument must be an unsigned integer").ThrowAsJavaScriptException();

			return;
		} else {
			if (uint32_t(info[1].As<Napi::Number>()) == 2)
				family = AF_INET6;
		}
	}
	
	socket->family_ = family;
	
	socket->poll_initialised_ = false;
	
	socket->no_ip_header_ = false;

	rc = socket->CreateSocket ();
	if (rc != 0) {
		Napi::Error::New(env, raw_strerror (rc)).ThrowAsJavaScriptException();

		return;
	}
}

void SocketWrap::OnClose (uv_handle_t *handle) {
	delete handle;
}

Napi::Value SocketWrap::Pause(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	SocketWrap* socket = this;

	if (info.Length () < 2) {
		Napi::Error::New(env, "Two arguments are required").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	if (! info[0].IsBoolean ()) {
		Napi::TypeError::New(env, "Recv argument must be a boolean").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	bool pause_recv = info[0].As<Napi::Boolean>();

	if (! info[1].IsBoolean ()) {
		Napi::TypeError::New(env, "Send argument must be a boolean").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	bool pause_send = info[1].As<Napi::Boolean>();
	
	int events = (pause_recv ? 0 : UV_READABLE)
			| (pause_send ? 0 : UV_WRITABLE);

	if (! socket->deconstructing_ && socket->poll_initialised_) {
		uv_poll_stop (socket->poll_watcher_);
		if (events)
			uv_poll_start (socket->poll_watcher_, events, IoEvent);
	}
	
	return info.This();
}

Napi::Value SocketWrap::Recv(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	SocketWrap* socket = this;
	Napi::Buffer<char> buffer;
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
		Napi::Error::New(env, "Five arguments are required").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	if (! info[0].IsBuffer()) {
		Napi::TypeError::New(env, "Buffer argument must be a node Buffer object").ThrowAsJavaScriptException();

		return env.Undefined();
	} else {
		buffer = info[0].As<Napi::Buffer<char> >();
	}

	if (! info[1].IsFunction ()) {
		Napi::TypeError::New(env, "Callback argument must be a function").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	rc = socket->CreateSocket ();
	if (rc != 0) {
		Napi::Error::New(env, raw_strerror (errno)).ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (socket->family_ == AF_INET6) {
		memset (&sin6_address, 0, sizeof (sin6_address));
		rc = recvfrom (socket->poll_fd_, buffer.Data(),
				(int) buffer.Length(), 0, (sockaddr *) &sin6_address,
				&sin_length);
	} else {
		memset (&sin_address, 0, sizeof (sin_address));
		rc = recvfrom (socket->poll_fd_, buffer.Data(),
				(int) buffer.Length(), 0, (sockaddr *) &sin_address,
				&sin_length);
	}
	
	if (rc == SOCKET_ERROR) {
		Napi::Error::New(env, raw_strerror (SOCKET_ERRNO)).ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	if (socket->family_ == AF_INET6)
		uv_ip6_name (&sin6_address, addr, 50);
	else
		uv_ip4_name (&sin_address, addr, 50);
	
	Napi::Function cb = info[1].As<Napi::Function>();
	const unsigned argc = 3;
	Napi::Value argv[argc];
	argv[0] = info[0];
	argv[1] = Napi::Number::New(env, rc);
	argv[2] = Napi::String::New(env, addr);
	cb.Call(std::initializer_list<napi_value>{ argv[0], argv[1], argv[2] });
	
	return info.This();
}

Napi::Value SocketWrap::Send(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	SocketWrap* socket = this;
	Napi::Buffer<char> buffer;
	uint32_t offset;
	uint32_t length;
	int rc;
	char *data;
	
	if (info.Length () < 5) {
		Napi::Error::New(env, "Five arguments are required").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	if (! info[0].IsBuffer()) {
		Napi::TypeError::New(env, "Buffer argument must be a node Buffer object").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	if (! info[1].IsNumber ()) {
		Napi::TypeError::New(env, "Offset argument must be an unsigned integer").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[2].IsNumber ()) {
		Napi::TypeError::New(env, "Length argument must be an unsigned integer").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[3].IsString ()) {
		Napi::TypeError::New(env, "Address argument must be a string").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[4].IsFunction ()) {
		Napi::TypeError::New(env, "Callback argument must be a function").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	rc = socket->CreateSocket ();
	if (rc != 0) {
		Napi::Error::New(env, raw_strerror (errno)).ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	buffer = info[0].As<Napi::Buffer<char> >();
	offset = info[1].As<Napi::Number>();
	length = info[2].As<Napi::Number>();

	data = buffer.Data() + offset;
	
	if (socket->family_ == AF_INET6) {
#if UV_VERSION_MAJOR > 0
		struct sockaddr_in6 addr;

		uv_ip6_addr(info[3].As<Napi::String>().Utf8Value().c_str(), 0, &addr);
#else
		String::Utf8String address (args[3]);
		struct sockaddr_in6 addr = uv_ip6_addr (*address, 0);
#endif
		
		rc = sendto (socket->poll_fd_, data, length, 0,
				(struct sockaddr *) &addr, sizeof (addr));
	} else {
#if UV_VERSION_MAJOR > 0
		struct sockaddr_in addr;
		uv_ip4_addr(info[3].As<Napi::String>().Utf8Value().c_str(), 0, &addr);
#else
		String::Utf8String address (info[3]);
		struct sockaddr_in addr = uv_ip4_addr (*address, 0);
#endif

		rc = sendto (socket->poll_fd_, data, length, 0,
				(struct sockaddr *) &addr, sizeof (addr));
	}
	
	if (rc == SOCKET_ERROR) {
		Napi::Error::New(env, raw_strerror (SOCKET_ERRNO)).ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	Napi::Function cb = info[4].As<Napi::Function>();
	const unsigned argc = 1;
	Napi::Value argv[argc];
	argv[0] = Napi::Number::New(env, rc);
	
	cb.Call(std::initializer_list<napi_value>{ argv[0] });
	return info.This();
}

Napi::Value SocketWrap::SetOption(const Napi::CallbackInfo& info) {
	Napi::Env env = info.Env();
	Napi::HandleScope scope(env);
	
	SocketWrap* socket = this;
	
	if (info.Length () < 3) {
		Napi::Error::New(env, "Three or four arguments are required").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[0].IsNumber ()) {
		Napi::TypeError::New(env, "Level argument must be a number").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	if (! info[1].IsNumber ()) {
		Napi::TypeError::New(env, "Option argument must be a number").ThrowAsJavaScriptException();

		return env.Undefined();
	}

	int level = info[0].As<Napi::Number>();
	int option = info[1].As<Napi::Number>();
	SOCKET_OPT_TYPE val = NULL;
	unsigned int ival = 0;
	SOCKET_LEN_TYPE len;

	if (info.Length () > 3) {
		if (! info[2].IsBuffer()) {
			Napi::TypeError::New(env, "Value argument must be a node Buffer object if length is provided").ThrowAsJavaScriptException();

			return env.Undefined();
		}
		
		Napi::Buffer<char> buffer = info[2].As<Napi::Buffer<char> >();
		val = buffer.Data();

		if (! info[3].IsNumber ()) {
			Napi::TypeError::New(env, "Length argument must be an unsigned integer").ThrowAsJavaScriptException();

			return env.Undefined();
		}

		len = info[3].As<Napi::Number>();

		if (len > buffer.Length()) {
			Napi::TypeError::New(env, "Length argument is larger than buffer length").ThrowAsJavaScriptException();

			return env.Undefined();
		}
	} else {
		if (! info[2].IsNumber ()) {
			Napi::TypeError::New(env, "Value argument must be a unsigned integer").ThrowAsJavaScriptException();

			return env.Undefined();
		}

		ival = info[2].As<Napi::Number>();
		len = 4;
	}

	int rc = setsockopt (socket->poll_fd_, level, option,
			(val ? val : (SOCKET_OPT_TYPE) &ival), len);

	if (rc == SOCKET_ERROR) {
		Napi::Error::New(env, raw_strerror(SOCKET_ERRNO)).ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	return info.This();
}

static void IoEvent (uv_poll_t* watcher, int status, int revents) {
	SocketWrap *socket = static_cast<SocketWrap*>(watcher->data);
	socket->HandleIOEvent (status, revents);
}

}; /* namespace raw */

#endif /* RAW_CC */
