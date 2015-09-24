#ifndef RAW_H
#define RAW_H

/**
 ** The following warnings are displayed during compilation on win32 platforms
 ** using node-gyp:
 **
 **  - C++ exception handler used, but unwind semantics are not enabled.
 **  - no definition for inline function 'v8::Persistent<T> \
 **       v8::Persistent<T>::New(v8::Handle<T>)'
 **
 ** There don't seem to be any issues which would suggest these are real
 ** problems, so we've disabled them for now.
 **/
#ifdef _WIN32
#pragma warning(disable:4506;disable:4530)
#endif

#include <string>

#include <node.h>
#include <node_buffer.h>
#include <nan.h>

#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#define SOCKET_ERRNO WSAGetLastError()
#define SOCKET_OPT_TYPE char *
#define SOCKET_LEN_TYPE int
#else
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#define SOCKET int
#define SOCKET_ERROR -1
#define SOCKET_ERRNO errno
#define INVALID_SOCKET -1
#define closesocket close
#define SOCKET_OPT_TYPE void *
#define SOCKET_LEN_TYPE socklen_t
#endif

using namespace v8;

namespace raw {

NAN_METHOD(CreateChecksum);

void ExportConstants (Handle<Object> target);
void ExportFunctions (Handle<Object> target);

NAN_METHOD(Htonl);
NAN_METHOD(Htons);
NAN_METHOD(Ntohl);
NAN_METHOD(Ntohs);

class SocketWrap : public Nan::ObjectWrap {
public:
	void HandleIOEvent (int status, int revents);
	static void Init (Handle<Object> exports);

private:
	SocketWrap ();
	~SocketWrap ();

	static NAN_METHOD(Close);

	void CloseSocket (void);
	
	int CreateSocket (void);

	static NAN_METHOD(GetOption);

	static NAN_METHOD(New);

	static void OnClose (uv_handle_t *handle);

	static NAN_METHOD(Pause);
	static NAN_METHOD(Recv);
	static NAN_METHOD(Send);
	static NAN_METHOD(SetOption);

	bool no_ip_header_;

	uint32_t family_;
	uint32_t protocol_;

	SOCKET poll_fd_;
	uv_poll_t *poll_watcher_;
	bool poll_initialised_;
	
	bool deconstructing_;
};

static void IoEvent (uv_poll_t* watcher, int status, int revents);

}; /* namespace raw */

#endif /* RAW_H */
