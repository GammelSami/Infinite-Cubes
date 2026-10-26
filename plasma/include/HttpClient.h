// A minimal HTTP/1.0 client over WinSock: one global instance (created by the workspace code) fetches a path from
// the activation server, or the release notes (/release.xml) and updates from the download host, either
// synchronously or on a worker thread, into a stream.
#pragma once
#ifndef PLASMA_NO_INTERNET
#include <winsock2.h>
#endif
#include <windows.h>
#include <iosfwd>
#include <string>

class HttpClient {
public:
    HttpClient(const std::string &host, const std::string &protocol);
    virtual ~HttpClient();

    // Sends "GET path" to the host and copies the response body to out; false on a connection error, a status other
    // than 200 or a cancel.
    bool download(const std::string &path, std::ostream &out);

    // Downloads path into out on a worker thread through the global client.
    static void __stdcall startDownload(const std::string &path, std::ostream &out);

    // Cancels the global client's download and waits for its thread to end.
    static void cancelDownload();
    // A worker thread is downloading (Workspace::Render redraws meanwhile).
    bool isRunning() const { return m_running; }
    // The response's Content-Length and the body bytes received so far provide download progress and detect short
    // responses.
    unsigned int getContentLength() const { return m_contentLength; }
    // The content length and byte count are unsigned.
    unsigned int getReceived() const { return m_received; }
    // Whether the worker thread's download failed.
    bool hasFailed() const { return m_failed; }

private:
    static void __cdecl downloadThread(void *argument);

    std::string m_path;  // The path the worker thread downloads.
    std::ostream *m_out; // The worker thread's output stream.
    std::string m_host;
    std::string m_protocol;       // "http"; not read in HttpClient.cpp.
    unsigned int m_contentLength; // From the Content-Length header.
    unsigned int m_received;      // Body bytes received so far.
    bool m_running;               // A worker thread is downloading.
    bool m_failed;                // The worker thread's download failed.
    bool m_cancel;                // Asks a download to stop.
    int m_port;
    uintptr_t m_thread; // _beginthread's result.
};

extern HttpClient *g_httpClient;
