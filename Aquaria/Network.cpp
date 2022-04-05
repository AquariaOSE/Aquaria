#include "minihttp.h"
#include "DSQ.h"
#include "Network.h"
#include "ByteBuffer.h"

#include "MT.h"
#include <map>
#include <set>
#include <cassert>
#include "SDL.h"

using namespace minihttp;

namespace Network {

struct RequestDataHolder
{
	RequestDataHolder() {}
	RequestDataHolder(RequestData *rq) : rq(rq), recvd(rq->_th_recvd), total(rq->_th_total)
	{
		if(rq->_th_aborted || rq->fail)
			ev = NE_ABORT;
		else if(rq->_th_finished)
			ev = NE_FINISH;
		else
			ev = NE_UPDATE;
	}
	RequestData *rq;
	NetEvent ev;
	size_t recvd;
	size_t total;
};

// Stores requests which have something interesting
static LockedQueue<RequestDataHolder> notifyRequests;


class HttpDumpSocket : public HttpSocket
{
public:

	virtual ~HttpDumpSocket() {}

protected:
	virtual void _OnClose()
	{

		minihttp::HttpSocket::_OnClose();

		const Request& r = GetCurrentRequest();
		RequestData *data = (RequestData*)(r.user);
		if(!data->_th_finished)
		{
			data->_th_aborted = true;
			notifyRequests.push(RequestDataHolder(data));
		}
	}
	virtual void _OnOpen()
	{

		minihttp::HttpSocket::_OnOpen();

		//const Request& r = GetCurrentRequest();
		// TODO ??
	}

	virtual void _OnRequestDone()
	{
		const Request& r = GetCurrentRequest();
		RequestData *data = (RequestData*)(r.user);

		if(data->fp)
		{
			fclose(data->fp);
			data->fp = NULL;
		}
		if(data->tempFilename != data->finalFilename)
		{
			if(rename(data->tempFilename.c_str(), data->finalFilename.c_str()))
			{
				perror("SOCKET: _OnRequestDone() failed to rename file");
				data->fail = true;
			}
		}
		data->_th_finished = true;
		data->_th_aborted = (GetStatusCode() != minihttp::HTTP_OK);
		notifyRequests.push(RequestDataHolder(data));
	}

	virtual void _OnRecv(void *buf, unsigned int size)
	{
		if(!size)
			return;

		const Request& r = GetCurrentRequest();
		RequestData *data = (RequestData*)(r.user);
		if(!data->fp && !data->fail)
		{
			data->fp = fopen(data->tempFilename.c_str(), "wb");
			if(!data->fp)
			{
				fprintf(stderr, "SOCKET: Failed to save %u bytes, file not open", size);
				data->fail = true;
				// TODO: and now?
				return;
			}
		}
		fwrite(buf, 1, size, data->fp);
		data->_th_recvd += size;
		data->_th_total = GetContentLen(); // 0 if chunked transfer encoding is used.
		notifyRequests.push(RequestDataHolder(data));
	}
};

// for first-time init, and signal to shut down worker thread
static volatile bool netUp = false;

// Used when sending a HTTP request
static std::string userAgent;

// socket updater thread
static SDL_Thread *worker = NULL;

// Request Queue (filled by download(), emptied by the worker thread)
static LockedQueue<RequestData*> RQ;

static int _NetworkWorkerThread(void *); // pre-decl

static void init()
{
	if(netUp)
		return;

	puts("NETWORK: Init");

	std::ostringstream os;
	os << "Aquaria";
#ifdef AQUARIA_DEMO
	os << " Demo";
#endif
	os << " v" << VERSION_MAJOR << "." << VERSION_MINOR << "." << VERSION_REVISION;
#ifdef AQUARIA_CUSTOM_BUILD_ID
	os << AQUARIA_CUSTOM_BUILD_ID;
#endif
#ifdef AQUARIA_OVERRIDE_VERSION_STRING
	os << "|" << AQUARIA_OVERRIDE_VERSION_STRING;
#endif

	const char *loc = getUsedLocale();
	if(*loc)
		os << "; Locale=" << loc;

	userAgent = os.str();

	if(!worker)
	{
#if SDL_VERSION_ATLEAST(2,0,0)
		worker = SDL_CreateThread(_NetworkWorkerThread, "network", NULL);
#else
		worker = SDL_CreateThread(_NetworkWorkerThread, NULL);
#endif
	}
}

void shutdown()
{
	if(netUp)
	{
		netUp = false;
		puts("NETWORK: Waiting for thread to exit...");
		SDL_WaitThread(worker, NULL);
		worker = NULL;
	}
}

// stores all sockets currently in use
// Accessed by worker thread ONLY!
static minihttp::SocketSet sockets;


static HttpDumpSocket *th_CreateSocket()
{
	HttpDumpSocket *sock = new HttpDumpSocket;
	sock->SetAlwaysHandle(false); // only handle incoming data on success
	sock->SetBufsizeIn(1024 * 16);
	sock->SetKeepAlive(0);
	sock->SetNonBlocking(true);
	sock->SetUserAgent(userAgent);
	sock->SetFollowRedirect(true);
	sockets.add(sock, true);
	return sock;
}


// must only be run by _NetworkWorkerThread
static bool th_DoSendRequest(RequestData *rq)
{
	Request get;
	SplitURI(rq->url, get.protocol, get.host, get.resource, get.port, get.useSSL);
	if(get.port < 0)
		get.port = 80;

	std::ostringstream hostdesc;
	hostdesc << get.host << ':' << get.port;

	HttpDumpSocket *sock = th_CreateSocket();

	get.user = rq;
	return sock->SendRequest(get, false);
}

static int _NetworkWorkerThread(void *)
{
	// Init & shutdown networking from the same thread.
	// I vaguely remember this could cause trouble on win32 otherwise. -- fg
	if(!(netUp = InitNetwork()))
	{
		fprintf(stderr, "NETWORK: Failed to init network\n");
		return -1;
	}

	RequestData *rq;
	while(netUp)
	{
		while(RQ.pop(rq))
		{
			if(!th_DoSendRequest(rq))
			{
				rq->_th_aborted = true;
				notifyRequests.push(RequestDataHolder(rq));
			}
		}
		while(sockets.update()) {}
		SDL_Delay(10);
	}
	puts("Network worker thread exiting");
	StopNetwork();
	return 0;
}

void download(RequestData *rq)
{
	init();
	RQ.push(rq);
}

void update()
{
	if(!netUp)
		return;

	RequestDataHolder h;
	while(notifyRequests.pop(h))
		h.rq->notify(h.ev, h.recvd, h.total);
}


} // namespace Network
