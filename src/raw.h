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

#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#define SOCKET_ERRNO WSAGetLastError()
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
#endif

using namespace v8;

namespace raw {

class SocketWrap : public node::ObjectWrap {
public:
	void HandleIOEvent (int status, int revents);
	static void Init (Handle<Object> target);

private:
	SocketWrap ();
	~SocketWrap ();
	
	static Handle<Value> Close (const Arguments& args);
	
	void CloseSocket (void);
	int CreateSocket (void);
	
	static Handle<Value> GenerateChecksums (const Arguments& args);
	
	static void OnClose (uv_handle_t *handle);
	
	static Handle<Value> New (const Arguments& args);
	static Handle<Value> NoIpHeader (const Arguments& args);
	static Handle<Value> Pause (const Arguments& args);
	static Handle<Value> Recv (const Arguments& args);
	static Handle<Value> Send (const Arguments& args);
	
	bool generate_checksums_;
	unsigned int checksum_offset_;
	
	bool no_ip_header_;
	
	uint32_t family_;
	uint32_t protocol_;
	
	SOCKET poll_fd_;
	uv_poll_t poll_watcher_;
	bool poll_initialised_;
};

static void IoEvent (uv_poll_t* watcher, int status, int revents);

}; /* namespace raw */

#endif /* RAW_H */
