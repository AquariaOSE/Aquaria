/* This program is free software. It comes without any warranty, to
* the extent permitted by applicable law. You can redistribute it
* and/or modify it under the terms of the Do What The Fuck You Want
* To Public License, Version 2, as published by Sam Hocevar.
* See http://sam.zoy.org/wtfpl/COPYING for more details. */

#ifndef MINIHTTPSOCKET_H
#define MINIHTTPSOCKET_H


// ---- Compile config -----
#define MINIHTTP_SUPPORT_HTTP
#define MINIHTTP_SUPPORT_SOCKET_SET
// -------------------------


#include <string>

#ifndef _WIN32
#  include <stdint.h>
#endif

namespace minihttp
{

bool InitNetwork();
void StopNetwork();

bool SplitURI(const std::string& uri, std::string& host, std::string& file, int& port);


class TcpSocket
{
public:
    TcpSocket();
    virtual ~TcpSocket();

    virtual bool HasPendingTask() const { return false; }

    bool open(const char *addr = NULL, unsigned int port = 0);
    void close();
    bool update(); // returns true if something interesting happened (incoming data, closed connection, etc)

    bool isOpen(void);

    void SetBufsizeIn(unsigned int s);
    bool SetNonBlocking(bool nonblock);
    unsigned int GetBufSize() { return _inbufSize; }
    const char *GetHost(void) { return _host.c_str(); }
    bool SendBytes(const char *str, unsigned int len);

protected:
    virtual void _OnCloseInternal();
    virtual void _OnData(); // data received callback. Internal, should only be overloaded to call _OnRecv()

    virtual void _OnRecv(char *buf, unsigned int size) = 0;
    virtual void _OnClose() {}; // close callback
    virtual void _OnOpen() {} // called when opened
    virtual bool _OnUpdate() { return true; } // called before reading from the socket

    void _ShiftBuffer();

    char *_inbuf;
    char *_readptr; // part of inbuf, optionally skipped header
    char *_writeptr; // passed to recv(). usually equal to _inbuf, but may point inside the buffer in case of a partial transfer.

    unsigned int _inbufSize; // size of internal buffer
    unsigned int _writeSize; // how many bytes can be written to _writeptr;
    unsigned int _recvSize; // incoming size, max _inbufSize - 1

    unsigned int _lastport; // port used in last open() call

    bool _nonblocking; // Default true. If false, the current thread is blocked while waiting for input.

    intptr_t _s; // socket handle. really an int, but to be sure its 64 bit compatible as it seems required on windows, we use this.

    std::string _host;
};

} // end namespace minihttp


// ------------------------------------------------------------------------

#ifdef MINIHTTP_SUPPORT_HTTP

#include <map>
#include <queue>

namespace minihttp
{

enum HttpCode
{
    HTTP_OK = 200,
    HTTP_NOTFOUND = 404,
};

struct Request
{
    Request() : port(80), user(NULL) {}
    Request(const std::string& h, const std::string& res, int p = 80, void *u = NULL)
        : host(h), resource(res), port(80), user(u) {}

    std::string host;
    std::string header; // set by socket
    std::string resource;
    int port;
    void *user;
};

class HttpSocket : public TcpSocket
{
public:

    HttpSocket();
    virtual ~HttpSocket();

    virtual bool HasPendingTask() const
    {
        return ExpectMoreData() || _requestQ.size();
    }

    void SetKeepAlive(unsigned int secs) { _keep_alive = secs; }
    void SetUserAgent(const std::string &s) { _user_agent = s; }
    void SetAcceptEncoding(const std::string& s) { _accept_encoding = s; }
    void SetFollowRedirect(bool follow) { _followRedir = follow; }
    void SetAlwaysHandle(bool h) { _alwaysHandle = h; }

    bool Download(const std::string& url, void *user = NULL);
    bool SendGet(Request& what, bool enqueue);
    bool SendGet(const std::string what, void *user = NULL);
    bool QueueGet(const std::string what, void *user = NULL);

    unsigned int GetRemaining() const { return _remaining; }

    unsigned int GetStatusCode() const { return _status; }
    unsigned int GetContentLen() const { return _contentLen; }
    bool ChunkedTransfer() const { return _chunkedTransfer; }
    bool ExpectMoreData() const { return _remaining || _chunkedTransfer; }

    const Request &GetCurrentRequest() const { return _curRequest; }
    const char *Hdr(const char *h) const;

    bool IsRedirecting() const;

protected:
    virtual void _OnCloseInternal();
    virtual void _OnClose();
    virtual void _OnData(); // data received callback. Internal, should only be overloaded to call _OnRecv()
    virtual void _OnRecv(char *buf, unsigned int size) = 0;
    virtual void _OnOpen(); // called when opene
    virtual bool _OnUpdate(); // called before reading from the socket

    // new ones:
    virtual void _OnRequestDone() {}

    void _ProcessChunk();
    bool _EnqueueOrSend(const Request& req, bool forceQueue = false);
    void _DequeueMore();
    bool _OpenRequest(const Request& req);
    void _ParseHeader();
    void _ParseHeaderFields(const char *s, size_t size);
    bool _HandleStatus(); // Returns whether the processed request was successful, or not
    void _FinishRequest();
    void _OnRecvInternal(char *buf, unsigned int size);

    std::string _user_agent;
    std::string _accept_encoding; // Default empty.
    std::string _tmpHdr; // used to save the http header if the incoming buffer was not large enough

    unsigned int _keep_alive; // http related
    unsigned int _remaining; // http "Content-Length: X" - already recvd. 0 if ready for next packet.
                             // For chunked transfer encoding, this holds the remaining size of the current chunk
    unsigned int _contentLen; // as reported by server
    unsigned int _status; // http status code, HTTP_OK if things are good

    std::queue<Request> _requestQ;
    std::map<std::string, std::string> _hdrs; // Maps HTTP header fields to their values

    Request _curRequest;

    bool _inProgress;
    bool _chunkedTransfer;
    bool _mustClose; // keep-alive specified, or not
    bool _followRedir; // Default true. Follow 3xx redirects if this is set.
    bool _alwaysHandle; // Also deliver to _OnRecv() if a non-success code was received.
};

} // end namespace minihttp

#endif

// ------------------------------------------------------------------------

#ifdef MINIHTTP_SUPPORT_SOCKET_SET

#include <map>

namespace minihttp
{

class SocketSet
{
public:
    virtual ~SocketSet();
    void deleteAll();
    bool update();
    void add(TcpSocket *s, bool deleteWhenDone = true);
    bool has(TcpSocket *s);
    void remove(TcpSocket *s);
    inline size_t size() { return _store.size(); }

//protected:

    struct SocketSetData
    {
        bool deleteWhenDone;
        // To be extended
    };
    
    typedef std::map<TcpSocket*, SocketSetData> Store;

    Store _store;
};

#endif


} // end namespace minihttp


#endif
