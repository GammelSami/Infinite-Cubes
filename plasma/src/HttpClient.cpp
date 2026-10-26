#ifndef PLASMA_NO_INTERNET
#include "HttpClient.h"
#include <process.h>
#include <sstream>

static const int DEFAULT_PORT = 80;
static const unsigned HTTP_OK = 200;
static const int RECEIVE_BUFFER_SIZE = 1024;
static const DWORD CANCEL_POLL_MILLISECONDS = 200;

HttpClient *g_httpClient;

HttpClient::HttpClient(const std::string &host, const std::string &protocol)
    : m_host(host),
      m_protocol(protocol),
      m_contentLength(0),
      m_received(0),
      m_running(false),
      m_failed(false),
      m_cancel(false),
      m_port(DEFAULT_PORT),
      m_thread(0)
{}

HttpClient::~HttpClient() {}

// Reads one line up to '\n' (the '\n' is dropped, a '\r' kept).
static std::string readLine(SOCKET s)
{
    std::string line;
    int received;
    do {
        char c;
        received = recv(s, &c, 1, 0);

        // Uninitialized read when recv fails: c is tested regardless, as in the code.
        if (c == '\n')
            break;
        line += c;
    } while (received > 0);
    return line;
}

bool HttpClient::download(const std::string &path, std::ostream &out)
{
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
        return false;

    unsigned long address = inet_addr(m_host.c_str());
    if (address == INADDR_NONE) {
        hostent *host = gethostbyname(m_host.c_str());
        if (host)
            address = *(unsigned long *)host->h_addr_list[0];
    }
    sockaddr_in server;
    memset(&server, 0, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = address;
    server.sin_port = htons(m_port);
    if (connect(s, (sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        closesocket(s);
        return false;
    }

    std::string request;
    request += "GET " + path + " HTTP/1.0\r\n";
    request += "Host: " + m_host + "\r\n";
    request += "Accept: */*\r\n";
    request += "Connection: close\r\n\r\n";
    if (send(s, request.c_str(), request.size(), 0) != request.size()) {
        closesocket(s);
        return false;
    }

    m_contentLength = 0;
    std::string line = readLine(s);
    std::stringstream status(line);
    std::string protocol;
    status >> protocol;
    unsigned int code;
    status >> code;
    bool invalid = line.empty() || protocol.substr(0, 5) != "HTTP/";
    if (invalid || code != HTTP_OK) {
        closesocket(s);
        return false;
    }

    // Headers up to the empty line; only Content-Length is kept.
    do {
        if (m_cancel) {
            closesocket(s);
            return false;
        }
        line = readLine(s);
        std::stringstream header(line);
        std::string name;
        header >> name;
        if (name == "Content-Length:")
            header >> m_contentLength;
    } while (!line.empty() && line != "\r");

    m_received = 0;
    char buffer[RECEIVE_BUFFER_SIZE];
    for (;;) {
        if (m_cancel) {
            closesocket(s);
            return false;
        }
        int received = recv(s, buffer, sizeof(buffer), 0);
        m_received += received;
        if (received <= 0)
            break;
        out.write(buffer, received);
    }
    closesocket(s);
    return true;
}

void __cdecl HttpClient::downloadThread(void *argument)
{
    HttpClient *client = (HttpClient *)argument;
    try {
        if (!client->download(client->m_path, *client->m_out))
            client->m_failed = true;
    } catch (std::exception &e) {
        client->m_failed = true;
    }
    client->m_running = false;
}

void __stdcall HttpClient::startDownload(const std::string &path, std::ostream &out)
{
    HttpClient *client = g_httpClient;
    client->m_path = path;
    client->m_out = &out;
    client->m_running = true;
    client->m_failed = false;
    client->m_thread = _beginthread(downloadThread, 0, client);
}

void HttpClient::cancelDownload()
{
    HttpClient *client = g_httpClient;
    client->m_cancel = true;
    while (client->m_running)
        Sleep(CANCEL_POLL_MILLISECONDS);
    client->m_cancel = false;
}

#endif
