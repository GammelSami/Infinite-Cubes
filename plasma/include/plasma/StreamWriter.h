// StreamWriter.cpp: the base writer used by PLXStreamWriter.
#pragma once
#include <ostream>
#include <string>
#include <map>
#include <list>

namespace plasma {

// Numbered chunks with backpatched lengths and optional scrambled names.
class StreamWriter {
public:
    StreamWriter(std::ostream &stream, unsigned __int64 key);
    void beginChunk(const std::string &name, bool scrambleName);
    void defineChunk(const std::string &name, bool scrambleName);
    void beginChunkId(int id);
    void endChunk();
    void writeString(const std::string &text);
    void writeWString(const std::wstring &text);
    void obfuscate(char *data, int length, const unsigned __int64 &key);
    std::ostream &write4(const void *value);

    std::ostream *m_stream;
    std::map<std::string, int> m_chunkIds;
    int m_nextId;
    unsigned __int64 m_obfuscationKey;
    std::list<std::streampos> m_chunkStarts;
};

}
