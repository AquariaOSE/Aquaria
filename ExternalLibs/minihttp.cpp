// minihttp.cpp - All functionality required for a minimal TCP/HTTP client packed in one file.
// Released under the WTFPL (See minihttp.h)

#ifdef _MSC_VER
#  ifndef _CRT_SECURE_NO_WARNINGS
#    define _CRT_SECURE_NO_WARNINGS
#  endif
#  ifndef _CRT_SECURE_NO_DEPRECATE
#    define _CRT_SECURE_NO_DEPRECATE
#  endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sstream>
#include <cctype>
#include <cerrno>
#include <algorithm>

#ifdef _WIN32
#  ifndef _WIN32_WINNT
#    define _WIN32_WINNT 0x0501
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  define EWOULDBLOCK WSAEWOULDBLOCK
#  define ETIMEDOUT WSAETIMEDOUT
#  define ECONNRESET WSAECONNRESET
#  define ENOTCONN WSAENOTCONN
#else
#  include <sys/types.h>
#  include <unistd.h>
#  include <fcntl.h>
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  define SOCKET_ERROR (-1)
#  define INVALID_SOCKET (SOCKET)(~0)
   typedef intptr_t SOCKET;
#endif

#include "minihttp.h"

#define SOCKETVALID(s) ((s) != INVALID_SOCKET)

#ifdef _MSC_VER
#  define STRNICMP _strnicmp
#else
#  define STRNICMP strncasecmp
#endif

#ifdef _DEBUG
#  define traceprint(...) {printf(__VA_ARGS__);}
#else
#  define traceprint(...) {}
#endif

namespace minihttp {

#define DEFAULT_BUFSIZE 4096

inline int _GetError()
{
#ifdef _WIN32
    return WSAGetLastError();
#else
    return errno;
#endif
}

inline std::string _GetErrorStr(int e)
{
#ifdef _WIN32
    LPTSTR s;
    ::FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, e, 0, (LPTSTR)&s, 0, NULL);
    std::string ret = s;
    ::LocalFree(s);
    return ret;
#endif
    return strerror(e);
}

bool InitNetwork()
{
#ifdef _WIN32
    WSADATA wsadata;
    if(WSAStartup(MAKEWORD(2,2), &wsadata))
    {
        traceprint("WSAStartup ERROR: %s", _GetErrorStr(_GetError()).c_str());
        return false;
    }
#endif
    return true;
}

void StopNetwork()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

static bool _Resolve(const char *host, unsigned int port, struct sockaddr_in *addr)
{
    char port_str[15];
    sprintf(port_str, "%u", port);

    struct addrinfo hnt, *res = 0;
    memset(&hnt, 0, sizeof(hnt));
    hnt.ai_family = AF_INET;
    hnt.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(host, port_str, &hnt, &res))
    {
        traceprint("RESOLVE ERROR: %s", _GetErrorStr(_GetError()).c_str());
        return false;
    }
    if (res)
    {
        if (res->ai_family != AF_INET)
        {
            traceprint("RESOLVE WTF: %s", _GetErrorStr(_GetError()).c_str());
            freeaddrinfo(res);
            return false;
        }
        memcpy(addr, res->ai_addr, res->ai_addrlen);
        freeaddrinfo(res);
        return true;
    }
    return false;
}

// FIXME: this does currently not handle links like:
// http://example.com/index.html#pos

bool SplitURI(const std::string& uri, std::string& host, std::string& file, int& port)
{
    const char *p = uri.c_str();
    const char *sl = strstr(p, "//");
    unsigned int offs = 0;
    if(sl)
    {
        offs = 7;
        if(strncmp(p, "http://", offs))
            return false;
        p = sl + 2;
    }
    sl = strchr(p, '/');
    if(!sl)
    {
        host = p;
        file = "/";
    }
    else
    {
        host = uri.substr(offs, sl - p);
        file = sl;
    }

    port = -1;
    size_t colon = host.find(':');
    if(colon != std::string::npos)
    {
        port = atoi(host.c_str() + colon);
        host.erase(port);
    }

    return true;
}

static bool _SetNonBlocking(SOCKET s, bool nonblock)
{
    if(!SOCKETVALID(s))
        return false;
#ifdef _WIN32
    ULONG tmp = !!nonblock;
    if(::ioctlsocket(s, FIONBIO, &tmp) == SOCKET_ERROR)
        return false;
#else
    int tmp = ::fcntl(s, F_GETFL);
    if(tmp < 0)
        return false;
    if(::fcntl(s, F_SETFL, nonblock ? (tmp|O_NONBLOCK) : (tmp|=~O_NONBLOCK)) < 0)
        return false;
#endif
    return true;
}

TcpSocket::TcpSocket()
: _s(INVALID_SOCKET), _inbuf(NULL), _inbufSize(0), _recvSize(0),
  _readptr(NULL), _lastport(0)
{
}

TcpSocket::~TcpSocket()
{
    close();
    if(_inbuf)
        free(_inbuf);
}

bool TcpSocket::isOpen(void)
{
    return SOCKETVALID(_s);
}

void TcpSocket::close(void)
{
    if(!SOCKETVALID(_s))
        return;

    _OnCloseInternal();

#ifdef _WIN32
    ::closesocket((SOCKET)_s);
#else
    ::close(_s);
#endif
    _s = INVALID_SOCKET;
}

void TcpSocket::_OnCloseInternal()
{
    _OnClose();
}

bool TcpSocket::SetNonBlocking(bool nonblock)
{
    _nonblocking = nonblock;
    return _SetNonBlocking(_s, nonblock);
}

void TcpSocket::SetBufsizeIn(unsigned int s)
{
    if(s < 512)
        s = 512;
    if(s != _inbufSize)
        _inbuf = (char*)realloc(_inbuf, s);
    _inbufSize = s;
    _writeSize = s - 1;
    _readptr = _writeptr = _inbuf;
}

bool TcpSocket::open(const char *host /* = NULL */, unsigned int port /* = 0 */)
{
    if(isOpen())
    {
        if( (host && host != _host) || (port && port != _lastport) )
            close();
            // ... and continue connecting to new host/port
        else
            return true; // still connected, to same host and port.
    }

    sockaddr_in addr;

    if(host)
        _host = host;
    else
        host = _host.c_str();

    if(port)
        _lastport = port;
    else
    {
        port = _lastport;
        if(!port)
            return false;
    }

    if(!_Resolve(host, port, &addr))
    {
        traceprint("RESOLV ERROR: %s\n", _GetErrorStr(_GetError()).c_str());
        return false;
    }

    SOCKET s = socket(AF_INET, SOCK_STREAM, 0);

    if(!SOCKETVALID(s))
    {
        traceprint("SOCKET ERROR: %s\n", _GetErrorStr(_GetError()).c_str());
        return false;
    }

    if (::connect(s, (sockaddr*)&addr, sizeof(sockaddr)))
    {
        traceprint("CONNECT ERROR: %s\n", _GetErrorStr(_GetError()).c_str());
        return false;
    }

    _SetNonBlocking(s, _nonblocking); // restore setting if it was set in invalid state. static call because _s is intentionally still invalid here.
    _s = s; // set the socket handle when we are really sure we are connected, and things are set up

    _OnOpen();

    return true;
}

bool TcpSocket::SendBytes(const char *str, unsigned int len)
{
    if(!SOCKETVALID(_s))
        return false;
    //traceprint("SEND: '%s'\n", str);
    return ::send(_s, str, len, 0) >= 0;
    // TODO: check _GetError()
}

void TcpSocket::_ShiftBuffer(void)
{
    size_t by = _readptr - _inbuf;
    memmove(_inbuf, _readptr, by);
    _readptr = _inbuf;
    _writeptr = _inbuf + by;
    _writeSize = _inbufSize - by - 1;
}

void TcpSocket::_OnData()
{
    _OnRecv(_readptr, _recvSize);
}

bool TcpSocket::update(void)
{
   if(!_OnUpdate())
       return false;

   if(!isOpen())
       return false;

    if(!_inbuf)
        SetBufsizeIn(DEFAULT_BUFSIZE);

    int bytes = recv(_s, _writeptr, _writeSize, 0); // last char is used as string terminator

    if(bytes > 0) // we received something
    {
        _inbuf[bytes] = 0;
        _recvSize = bytes;

        // reset pointers for next read
        _writeSize = _inbufSize - 1;
        _readptr = _writeptr = _inbuf;

        _OnData();
        return true;
    }
    else if(bytes == 0) // remote has closed the connection
    {
        _recvSize = 0;
        close();
        return true;
    }
    else // whoops, error?
    {
        int e = _GetError();
        switch(e)
        {
        case ECONNRESET:
        case ENOTCONN:
        case ETIMEDOUT:
#ifdef _WIN32
        case WSAECONNABORTED:
        case WSAESHUTDOWN:
#endif
            close();
            break;

        case EWOULDBLOCK:
#if defined(EAGAIN) && (EWOULDBLOCK != EAGAIN)
        case EAGAIN: // linux man pages say this can also happen instead of EWOULDBLOCK
#endif
            return false;
        }
        traceprint("SOCKET UPDATE ERROR: (%d): %s\n", e, _GetErrorStr(e).c_str());
    }
    return true;
}


// ==========================
// ===== HTTP SPECIFIC ======
// ==========================
#ifdef MINIHTTP_SUPPORT_HTTP

static void strToLower(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), tolower);
}

HttpSocket::HttpSocket()
: TcpSocket(),
_keep_alive(0), _remaining(0), _chunkedTransfer(false), _mustClose(true), _inProgress(false),
_followRedir(true), _alwaysHandle(false), _status(0)
{
}

HttpSocket::~HttpSocket()
{
}

void HttpSocket::_OnOpen()
{
    TcpSocket::_OnOpen();
    _chunkedTransfer = false;
}

void HttpSocket::_OnCloseInternal()
{
    if(!IsRedirecting() || _alwaysHandle)
        _OnClose();
}

bool HttpSocket::_OnUpdate()
{
    if(!TcpSocket::_OnUpdate())
        return false;

    if(_inProgress && !_chunkedTransfer && !_remaining && _status)
        _FinishRequest();

    // initiate transfer if queue is not empty, but the socket somehow forgot to proceed
    if(_requestQ.size() && !_remaining && !_chunkedTransfer && !_inProgress)
        _DequeueMore();

    return true;
}

bool HttpSocket::Download(const std::string& url, void *user /* = NULL */)
{
    Request req;
    req.user = user;
    SplitURI(url, req.host, req.resource, req.port);
    if(req.port < 0)
        req.port = 80;
    return SendGet(req, false);
}

bool HttpSocket::SendGet(const std::string what, void *user /* = NULL */)
{
    Request req(what, _host, _lastport, user);
    return SendGet(req, false);
}

bool HttpSocket::QueueGet(const std::string what, void *user /* = NULL */)
{
    Request req(what, _host, _lastport, user);
    return SendGet(req, true);
}

bool HttpSocket::SendGet(Request& req, bool enqueue)
{
    if(req.host.empty() || !req.port)
        return false;

    std::stringstream r;
    const char *crlf = "\r\n";
    r << "GET " << req.resource << " HTTP/1.1" << crlf;
    r << "Host: " << req.host << crlf;
    if(_keep_alive)
    {
        r << "Connection: Keep-Alive" << crlf;
        r << "Keep-Alive: " << _keep_alive << crlf;
    }
    else
        r << "Connection: close" << crlf;

    if(_user_agent.length())
        r << "User-Agent: " << _user_agent << crlf;

    if(_accept_encoding.length())
        r << "Accept-Encoding: " << _accept_encoding << crlf;

    r << crlf; // header terminator

    req.header = r.str();

    return _EnqueueOrSend(req, enqueue);
}

bool HttpSocket::_EnqueueOrSend(const Request& req, bool forceQueue /* = false */)
{
    if(_inProgress || forceQueue) // do not send while receiving other data
    {
        traceprint("HTTP: Transfer pending; putting into queue. Now %u waiting.\n", (unsigned int)_requestQ.size()); // DEBUG
        _requestQ.push(req);
        return true;
    }
    // ok, we can send directly
    if(!_OpenRequest(req))
        return false;
    _inProgress = SendBytes(req.header.c_str(), req.header.length());
    return _inProgress;
}

// called whenever a request is finished completely and the socket checks for more things to send
void HttpSocket::_DequeueMore(void)
{
    _FinishRequest(); // In case this was not done yet.

    // _inProgress is known to be false here
    if(_requestQ.size()) // still have other requests queued?
        if(_EnqueueOrSend(_requestQ.front(), false)) // could we send?
            _requestQ.pop(); // if so, we are done with this request

    // otherwise, we are done for now. socket is kept alive for future sends. Nothing to do.
}

bool HttpSocket::_OpenRequest(const Request& req)
{
    if(_inProgress)
    {
        traceprint("HttpSocket::_OpenRequest(): _inProgress == true, should not be called.");
        return false;
    }
    if(!open(req.host.c_str(), req.port))
        return false;
    _inProgress = true;
    _curRequest = req;
    _status = 0;
    return true;
}

void HttpSocket::_FinishRequest(void)
{
    if(_inProgress)
    {
        if(!IsRedirecting() || _alwaysHandle)
            _OnRequestDone(); // notify about finished request
        _inProgress = false;
        _hdrs.clear();
    }
}

void HttpSocket::_ProcessChunk(void)
{
    if(!_chunkedTransfer)
        return;

    unsigned int chunksize = -1;

    while(true)
    {
        // less data required until chunk end than received, means the new chunk starts somewhere in the middle
        // of the received data block. finish this chunk first.
        if(_remaining)
        {
            if(_remaining <= _recvSize) // it contains the rest of the chunk, including CRLF
            {
                _OnRecvInternal(_readptr, _remaining - 2); // implicitly skip CRLF
                _readptr += _remaining;
                _recvSize -= _remaining;
                _remaining = 0; // done with this one.
                if(!chunksize) // and if chunksize was 0, we are done with all chunks.
                    break;
            }
            else // buffer did not yet arrive completely
            {
                _OnRecvInternal(_readptr, _recvSize);
                _remaining -= _recvSize;
                _recvSize = 0; // done with the whole buffer, but not with the chunk
                return; // nothing else to do here
            }
        }

        // each chunk identifier ends with CRLF.
        // if we don't find that, we hit the corner case that the chunk identifier was not fully received.
        // in that case, adjust the buffer and wait for the rest of the data to be appended
        char *term = strstr(_readptr, "\r\n");
        if(!term)
        {
            if(_recvSize) // if there is still something queued, move it to the left of the buffer and append on next read
                _ShiftBuffer();
            return;
        }
        term += 2; // skip CRLF
        
        // when we are here, the (next) chunk header was completely received.
        chunksize = strtoul(_readptr, NULL, 16);
        _remaining = chunksize + 2; // the http protocol specifies that each chunk has a trailing CRLF
        _recvSize -= (term - _readptr);
        _readptr = term;
    }

    if(!chunksize) // this was the last chunk, no further data expected unless requested
    {
        _chunkedTransfer = false;
        _DequeueMore();
        if(_recvSize)
            traceprint("_ProcessChunk: There are %u bytes left in the buffer, huh?\n", _recvSize);
        if(_mustClose)
            close();
    }
}

void HttpSocket::_ParseHeaderFields(const char *s, size_t size)
{
    // Field: Entry data\r\n

    const char *maxs = s + size;
    const char *colon, *entry;
    const char *entryEnd = s; // last char of entry data
    while(s < maxs)
    {
        while(isspace(*s))
        {
            ++s;
            if(s >= maxs)
                return;
        }
        colon = strchr(s, ':');
        if(!colon)
            return;
        entryEnd = strchr(colon, '\n');
        if(!entryEnd)
            return;
        while(entryEnd[-1] == '\n' || entryEnd[-1] == '\r')
            --entryEnd;
        entry = colon + 1;
        while(isspace(*entry))
        {
            ++entry;
            if(entry > entryEnd) // Field, but no entry? (Field:   \n\r)
            {
                s = entryEnd;
                continue;
            }
        }
        std::string field(s, colon - s);
        strToLower(field);
        _hdrs[field] = std::string(entry, entryEnd - entry);
        s = entryEnd;
    }
}

const char *HttpSocket::Hdr(const char *h) const
{
    std::map<std::string, std::string>::const_iterator it = _hdrs.find(h);
    return it == _hdrs.end() ? NULL : it->second.c_str();
}

static int safeatoi(const char *s)
{
    return s ? atoi(s) : 0;
}

bool HttpSocket::_HandleStatus()
{
    _remaining = _contentLen = safeatoi(Hdr("content-length"));

    const char *encoding = Hdr("transfer-encoding");
    _chunkedTransfer = encoding && !STRNICMP(encoding, "chunked", 7);
    
    const char *conn = Hdr("connection"); // if its not keep-alive, server will close it, so we can too
    _mustClose = !conn || STRNICMP(conn, "keep-alive", 10);

    if(!(_chunkedTransfer || _contentLen) && _status == 200)
        traceprint("_ParseHeader: Not chunked transfer and content-length==0, this will go fail");

    switch(_status)
    {
        case 200:
            return true;

        case 301:
        case 302:
        case 303:
        case 307:
        case 308:
            if(_followRedir)
                if(const char *loc = Hdr("location"))
                {
                    traceprint("Following HTTP redirect to: %s\n", loc);
                    Download(loc, _curRequest.user);
                }
            return false;

        default:
            return false;
    }
}

bool HttpSocket::IsRedirecting() const
{
	switch(_status)
	{
		case 301:
		case 302:
		case 303:
		case 307:
		case 308:
			return true;
	}
	return false;
}


void HttpSocket::_ParseHeader(void)
{
    _tmpHdr += _inbuf;
    const char *hptr = _tmpHdr.c_str();

    if((_recvSize >= 5 || _tmpHdr.size() >= 5) && memcmp("HTTP/", hptr, 5))
    {
        traceprint("_ParseHeader: not HTTP stream\n");
        return;
    }

    const char *hdrend = strstr(hptr, "\r\n\r\n");
    if(!hdrend)
    {
        traceprint("_ParseHeader: could not find end-of-header marker, or incomplete buf; delaying.\n");
        return;
    }

    //traceprint(hptr);

    hptr = strchr(hptr + 5, ' '); // skip "HTTP/", already known
    if(!hptr)
        return; // WTF?
    ++hptr; // number behind first space is the status code
    _status = atoi(hptr);

    // Default values
    _chunkedTransfer = false;
    _contentLen = 0; // yet unknown

    hptr = strstr(hptr, "\r\n");
    _ParseHeaderFields(hptr + 2, hdrend - hptr);

    // FIXME: return value indicates success.
    // Bail out on non-success, or at least make it so that _OnRecv() is not called.
    // (Unless an override bool is given that even non-successful answers get their data delivered!)
    _HandleStatus();

    // get ready
    _readptr = strstr(_inbuf, "\r\n\r\n") + 4; // skip double newline. must have been found in hptr earlier.
    _recvSize -= (_readptr - _inbuf); // skip the header part
    _tmpHdr.clear();
}

// generic http header parsing
void HttpSocket::_OnData(void)
{
    if(!(_chunkedTransfer || (_remaining && _recvSize)))
        _ParseHeader();

    if(_chunkedTransfer)
    {
        _ProcessChunk(); // first, try to finish one or more chunks
    }
    else if(_remaining && _recvSize) // something remaining? if so, we got a header earlier, but not all data
    {
        _remaining -= _recvSize;
        _OnRecvInternal(_readptr, _recvSize);

        if(int(_remaining) < 0)
        {
            traceprint("_OnRecv: _remaining wrap-around, huh??\n");
            _remaining = 0;
        }
        if(!_remaining) // received last block?
        {
            if(_mustClose)
                close();
            else
                _DequeueMore();
        }

        // nothing else to do here.
    }

    // otherwise, the server sent just the header, with the data following in the next packet
}

void HttpSocket::_OnClose()
{
    if(!ExpectMoreData())
        _FinishRequest();
}

void HttpSocket::_OnRecvInternal(char *buf, unsigned int size)
{
    if(_status == 200 || _alwaysHandle)
        _OnRecv(buf, size);
}

#endif

// ===========================
// ===== SOCKET SET ==========
// ===========================
#ifdef MINIHTTP_SUPPORT_SOCKET_SET

SocketSet::~SocketSet()
{
    deleteAll();
}

void SocketSet::deleteAll(void)
{
    for(Store::iterator it = _store.begin(); it != _store.end(); ++it)
        delete it->first;
    _store.clear();
}

bool SocketSet::update(void)
{
    bool interesting = false;
    Store::iterator it = _store.begin();
    for( ; it != _store.end(); )
    {
        TcpSocket *sock =  it->first;
        SocketSetData& sdata = it->second;
        interesting = sock->update() || interesting;
        if(sdata.deleteWhenDone && !sock->isOpen() && !sock->HasPendingTask())
        {
            delete sock;
            _store.erase(it++);
        }
        else
           ++it;
    }
    return interesting;
}

void SocketSet::remove(TcpSocket *s)
{
    _store.erase(s);
}

void SocketSet::add(TcpSocket *s, bool deleteWhenDone /* = true */)
{
    s->SetNonBlocking(true);
    SocketSetData sdata;
    sdata.deleteWhenDone = deleteWhenDone;
    _store[s] = sdata;
}

#endif


} // namespace minihttp
