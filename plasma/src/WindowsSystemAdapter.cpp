static const __int64 BufferHashInitialValue = 0x3ffffffffffe5;
static const int VolumeInformationNameCapacity = 256;
static const int VolumeInformationNameLimit = 255;

#ifndef PLASMA_NO_LICENSING
#include <windows.h>
#include <wbemcli.h>
#include <fstream>
#include <sstream>

#include "WindowsSystemAdapter.h"

// Widens a byte string character by character.
std::wstring widenString(const std::string &text);

typedef std::vector<char> Buffer;

WindowsSystemAdapter::~WindowsSystemAdapter()
{
    for (std::list<Buffer *>::iterator it = m_buffers.begin(); it != m_buffers.end(); ++it)
        delete *it;
}

int WindowsSystemAdapter::coInitializeEx(int reserved, int flags)
{
    return ::CoInitializeEx((void *)reserved, flags);
}

void WindowsSystemAdapter::coUninitialize()
{
    ::CoUninitialize();
}

int WindowsSystemAdapter::failed(int result)
{
    return result < 0;
}

int WindowsSystemAdapter::coInitializeSecurity(int securityDescriptor, int authServiceCount, int authServices,
                                               int reserved1, int authnLevel, int impLevel, int authList,
                                               int capabilities, int reserved3)
{
    return ::CoInitializeSecurity((PSECURITY_DESCRIPTOR)securityDescriptor, authServiceCount,
                                  (SOLE_AUTHENTICATION_SERVICE *)authServices, (void *)reserved1, authnLevel, impLevel,
                                  (void *)authList, capabilities, (void *)reserved3);
}

int WindowsSystemAdapter::createLocator(int outer, int context, int locator)
{
    return CoCreateInstance(CLSID_WbemLocator, (LPUNKNOWN)outer, context, IID_IWbemLocator, (LPVOID *)locator);
}

int WindowsSystemAdapter::connectServer(int locator, std::string resource, int user, int password, int locale,
                                        int securityFlags, int authority, int context, int services)
{
    return ((IWbemLocator *)locator)
        ->ConnectServer((BSTR)widenString(resource).c_str(), (BSTR)user, (BSTR)password, (BSTR)locale, securityFlags,
                        (BSTR)authority, (IWbemContext *)context, (IWbemServices **)services);
}

int WindowsSystemAdapter::release(int object)
{
    return ((IUnknown *)object)->Release();
}

int WindowsSystemAdapter::coSetProxyBlanket(int proxy, int authnService, int authzService, int serverPrincipalName,
                                            int authnLevel, int impLevel, int authInfo, int capabilities)
{
    return ::CoSetProxyBlanket((IUnknown *)proxy, authnService, authzService, (OLECHAR *)serverPrincipalName,
                               authnLevel, impLevel, (RPC_AUTH_IDENTITY_HANDLE)authInfo, capabilities);
}

int WindowsSystemAdapter::execQuery(int services, std::string language, std::string query, int flags, int context,
                                    int enumerator)
{
    return ((IWbemServices *)services)
        ->ExecQuery((BSTR)widenString(language).c_str(), (BSTR)widenString(query).c_str(), flags,
                    (IWbemContext *)context, (IEnumWbemClassObject **)enumerator);
}

int WindowsSystemAdapter::next(int enumerator, int timeout, int count, int objects, int returned)
{
    return ((IEnumWbemClassObject *)enumerator)->Next(timeout, count, (IWbemClassObject **)objects, (ULONG *)returned);
}

// The property as narrow text, or "" if it is missing or not a string.
std::string WindowsSystemAdapter::getProperty(int object, std::string name, int flags, int type, int flavor)
{
    VARIANT value;
    HRESULT result =
        ((IWbemClassObject *)object)->Get(widenString(name).c_str(), flags, &value, (CIMTYPE *)type, (long *)flavor);
    if (FAILED(result))
        return "";
    if (value.vt != VT_BSTR)
        return "";
    std::wstring text(value.bstrVal);
    VariantClear(&value);
    return std::string(text.begin(), text.end());
}

int WindowsSystemAdapter::newBuffer()
{
    m_buffers.push_back(new Buffer);
    return (int)m_buffers.back();
}

void WindowsSystemAdapter::freeBuffer(int buffer)
{
    Buffer *bytes = (Buffer *)buffer;
    m_buffers.remove(bytes);
    delete bytes;
}

void WindowsSystemAdapter::setByte(int buffer, int index, int value)
{
    (*(Buffer *)buffer)[index] = value;
}

int WindowsSystemAdapter::getByte(int buffer, int index)
{
    return (unsigned char)(*(Buffer *)buffer)[index];
}

int WindowsSystemAdapter::getSize(int buffer)
{
    return ((Buffer *)buffer)->size();
}

void WindowsSystemAdapter::setText(int buffer, std::string text)
{
    *(Buffer *)buffer = Buffer(text.begin(), text.end());
}

std::string WindowsSystemAdapter::getText(int buffer)
{
    return std::string(((Buffer *)buffer)->begin(), ((Buffer *)buffer)->end());
}

void WindowsSystemAdapter::resize(int buffer, int size)
{
    ((Buffer *)buffer)->resize(size);
}

// A new buffer with the file's contents (empty if it cannot be read).
int WindowsSystemAdapter::loadFile(std::string path)
{
    std::ifstream file(path, std::ios::binary);
    if (file.is_open()) {
        file.seekg(0, std::ios::end);
        int size = file.tellg();
        if (size > 0) {
            file.seekg(0, std::ios::beg);
            m_buffers.push_back(new Buffer(size));
            file.read(&(*m_buffers.back())[0], size);
        } else {
            m_buffers.push_back(new Buffer(0));
        }
    } else {
        m_buffers.push_back(new Buffer(0));
    }
    return (int)m_buffers.back();
}

void WindowsSystemAdapter::saveFile(int buffer, std::string path)
{
    std::ofstream file(path, std::ios::binary);
    Buffer &bytes = *(Buffer *)buffer;
    file.write(&bytes[0], bytes.size());
}

int WindowsSystemAdapter::openFile(std::string path)
{
    return (int)CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                            NULL);
}

void WindowsSystemAdapter::closeHandle(int file)
{
    ::CloseHandle((HANDLE)file);
}

int WindowsSystemAdapter::getCreationTimeHigh(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, &time, NULL, NULL);
    return time.dwHighDateTime;
}

int WindowsSystemAdapter::getCreationTimeLow(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, &time, NULL, NULL);
    return time.dwLowDateTime;
}

int WindowsSystemAdapter::getAccessTimeHigh(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, NULL, &time, NULL);
    return time.dwHighDateTime;
}

int WindowsSystemAdapter::getAccessTimeLow(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, NULL, &time, NULL);
    return time.dwLowDateTime;
}

int WindowsSystemAdapter::getWriteTimeHigh(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, NULL, NULL, &time);
    return time.dwHighDateTime;
}

int WindowsSystemAdapter::getWriteTimeLow(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, NULL, NULL, &time);
    return time.dwLowDateTime;
}

std::string WindowsSystemAdapter::getModulePath()
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH - 1);
    return path;
}

std::string WindowsSystemAdapter::getModuleDrive()
{
    std::string path = getModulePath();
    return path.substr(0, path.find("\\"));
}

// Replaces the buffer by an 8-byte hash of its bytes (h = 31 h + byte).
void WindowsSystemAdapter::hash(int buffer)
{
    Buffer &bytes = *(Buffer *)buffer;
    __int64 hash = BufferHashInitialValue;
    for (int i = 0; i < (int)bytes.size(); ++i)
        hash = hash * 31 + (unsigned char)bytes[i];
    bytes.resize(8);
    *(__int64 *)&bytes[0] = hash;
}

int WindowsSystemAdapter::compare(std::string left, std::string right)
{
    return left.compare(right);
}

std::string WindowsSystemAdapter::getVolumeSerialNumber()
{
    DWORD serialNumber;
    DWORD maximumComponentLength;
    DWORD fileSystemFlags;
    char fileSystemName[VolumeInformationNameCapacity];
    char volumeName[VolumeInformationNameCapacity];
    if (GetVolumeInformationA(NULL, volumeName, VolumeInformationNameLimit, &serialNumber, &maximumComponentLength,
                              &fileSystemFlags, fileSystemName, VolumeInformationNameLimit)) {
        std::stringstream text;
        text << serialNumber;
        return text.str();
    }
    return "";
}

#endif
