#ifndef AQ_NETWORK_H
#define AQ_NETWORK_H

namespace Network
{
	enum NetEvent
	{
		NE_UPDATE,
		NE_FINISH,
		NE_ABORT,
	};

	class RequestData
	{
	public:
		RequestData() : fp(0), fail(false), _th_finished(false), _th_aborted(false), _th_recvd(0), _th_total(0) {}
		virtual ~RequestData() {}
		virtual void notify(NetEvent ev, size_t recvd, size_t total) = 0;

		std::string url;
		unsigned int port;
		std::string tempFilename; // file name to write to while downloading
		std::string finalFilename; // name under which the file should be stored when finished
		FILE *fp;
		bool fail; // FIXME: really need this?

		// used internally by network namespace.
		// to avoid MT issues, these may not be accessed from outside!
		bool _th_finished;
		bool _th_aborted;
		size_t _th_recvd;
		size_t _th_total;
	};

	// Download a file described by rq.
	// Once the download is finished, rq's notify() method  will be called in the next update.
	void download(RequestData *rq);
	void update();
	void shutdown();
}


#endif
