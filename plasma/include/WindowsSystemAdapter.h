// WindowsSystemAdapter (WindowsSystemAdapter.cpp): the Windows services the abstr script program may call.
// Workspace.cpp keeps one global adapter and registers its methods as natives of the script machine
// (abstr::MethodN binders), so every argument and result is a script value: an int or a std::string. Integers
// stand for pointers on the script side: COM interfaces (WMI: IWbemLocator, IWbemServices, IEnumWbemClassObject,
// IWbemClassObject), file handles and byte buffers. A buffer is a std::vector<char> the adapter allocates and owns
// until FreeBuffer or its destruction.
#pragma once
#include <list>
#include <string>
#include <vector>

class WindowsSystemAdapter {
public:
    virtual ~WindowsSystemAdapter();

    // COM and WMI.
    int coInitializeEx(int reserved, int flags);
    void coUninitialize();
    int failed(int result);
    int coInitializeSecurity(int securityDescriptor, int authServiceCount, int authServices, int reserved1,
                             int authnLevel, int impLevel, int authList, int capabilities, int reserved3);
    int createLocator(int outer, int context, int locator);
    int connectServer(int locator, std::string resource, int user, int password, int locale, int securityFlags,
                      int authority, int context, int services);
    int release(int object);
    int coSetProxyBlanket(int proxy, int authnService, int authzService, int serverPrincipalName, int authnLevel,
                          int impLevel, int authInfo, int capabilities);
    int execQuery(int services, std::string language, std::string query, int flags, int context, int enumerator);
    int next(int enumerator, int timeout, int count, int objects, int returned);
    std::string getProperty(int object, std::string name, int flags, int type, int flavor);

    // Byte buffers.
    int newBuffer();
    void freeBuffer(int buffer);
    void setByte(int buffer, int index, int value);
    int getByte(int buffer, int index);
    int getSize(int buffer);
    void setText(int buffer, std::string text);
    std::string getText(int buffer);
    void resize(int buffer, int size);
    int loadFile(std::string path);
    void saveFile(int buffer, std::string path);

    // Files and the machine.
    int openFile(std::string path);
    void closeHandle(int file);
    int getCreationTimeHigh(int file);
    int getCreationTimeLow(int file);
    int getAccessTimeHigh(int file);
    int getAccessTimeLow(int file);
    int getWriteTimeHigh(int file);
    int getWriteTimeLow(int file);
    std::string getModulePath();
    std::string getModuleDrive(); // The module path up to its first backslash.
    void hash(int buffer);
    int compare(std::string left, std::string right);
    std::string getVolumeSerialNumber();

private:
    std::list<std::vector<char> *> m_buffers;
};
