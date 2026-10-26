#pragma once
#include <windows.h>
#include <iphlpapi.h>
#include <list>
#include <string>
#include <vector>
#include "abstr/Machine.h"

namespace cube {

// ReadAdapters' room for the network adapters' records.
enum {
    AdapterCapacity = 32
};

// System exposes script callbacks for files, MD5 hashing, network adapters, volume serial numbers
// and the executable path. Script values are integers or strings; integer values also carry
// pointers to owned byte buffers, adapter records and file handles.
class System {
public:
    explicit System(bool test);

    int newBuffer();                                                // "button:click"
    int loadBuffer(std::string path);                               // "button:hover"
    int bufferFromString(std::string text);                         // "edit:enter"
    void deleteBuffer(int buffer);                                  // "edit:leave"
    void setBufferByte(int buffer, int index, int value);           // "scroll:up"
    int bufferByte(int buffer, int index);                          // "scroll:down"
    int bufferSize(int buffer);                                     // "scroll:click"
    void resizeBuffer(int buffer, int size);                        // "window:resize"
    void appendInt(int buffer, int value);                          // "window:maximize"
    int readAdapters();                                             // "window:close"
    int firstAdapter();                                             // "window:minimize"
    int nextAdapter(int adapter);                                   // "screen:start"
    int adapterType(int adapter);                                   // "screen:options"
    int adapterAddressByte(int adapter, int index);                 // "screen:system"
    std::string fileHash(std::string path);                         // "screen:save"
    std::string stringHash(std::string text);                       // "checkbox:mark"
    std::string bufferHash(int buffer);                             // "checkbox:unmark"
    int openFile(std::string path);                                 // "static:enable"
    void closeFile(int file);                                       // "static:disable"
    int creationTimeHigh(int file);                                 // "static:1"
    int creationTimeLow(int file);                                  // "static:2"
    int accessTimeHigh(int file);                                   // "static:3"
    int accessTimeLow(int file);                                    // "static:4"
    int writeTimeHigh(int file);                                    // "static:5"
    int writeTimeLow(int file);                                     // "static:7"
    std::string modulePath();                                       // "widget:enter"
    std::string volumeSerial();                                     // "widget:leave"
    std::string concatenate(std::string first, std::string second); // "widget:show"
    void setTexts(std::string first, std::string second);           // "widget:hide"
    void setText(std::string text);                                 // "widget:enable"
    std::string bufferString(int buffer);                           // "widget:close"
    int equal(std::string first, std::string second);               // "widget:open"

    abstr::Machine m_machine;
    std::string m_firstText;                     // setTexts' first
    std::string m_secondText;                    // setTexts' second
    std::string m_text;                          // setText
    IP_ADAPTER_INFO m_adapters[AdapterCapacity]; // readAdapters
    std::list<std::vector<char> *> m_buffers;    // owned
    bool m_test;                                 // the constructor's argument, passed to the script's "test"
};

}
