#ifdef PLASMA_NO_INTERNET
#include "HttpClient.h"

// Fail closed: disabled downloads never create a socket or worker thread.
HttpClient *g_httpClient = 0;

HttpClient::HttpClient(const std::string &host, const std::string &protocol)
    : m_out(0),
      m_host(host),
      m_protocol(protocol),
      m_contentLength(0),
      m_received(0),
      m_running(false),
      m_failed(true),
      m_cancel(false),
      m_port(0),
      m_thread(0)
{}

HttpClient::~HttpClient() {}
bool HttpClient::download(const std::string &, std::ostream &)
{
    return false;
}
void __stdcall HttpClient::startDownload(const std::string &, std::ostream &) {}
void HttpClient::cancelDownload() {}
#endif
