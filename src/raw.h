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
 ** I've not yet worked out whether they are an issue or not so for now we've
 ** added the following for win32 to disable these warnings.
 **/
#ifdef _WIN32
#pragma warning(disable:4506;disable:4530)
#endif

#include <string>

#include <node.h>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <errno.h>
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
	
	static Handle<Value> New (const Arguments& args);
	static Handle<Value> Recv (const Arguments& args);
	static Handle<Value> Send (const Arguments& args);
	static Handle<Value> StopSendReady (const Arguments& args);
	static Handle<Value> StartSendReady (const Arguments& args);
	
	uint32_t protocol_;
	
	SOCKET poll_fd_;
	uv_poll_t poll_watcher_;
	bool poll_initialised_;
};

static void IoEvent (uv_poll_t* watcher, int status, int revents);

}; /* namespace raw */

#endif /* RAW_H */
