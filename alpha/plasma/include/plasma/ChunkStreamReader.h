// ChunkStreamReader decodes chunk headers, names and payload boundaries.
#pragma once
#include <istream>
#include <list>
#include <map>
#include <string>

namespace plasma {

class ChunkStreamReader {
public:
    explicit ChunkStreamReader(std::istream &stream);

    void deobfuscate(char *data, int length, const unsigned __int64 &key);
    void enterChunk();
    std::string parseChunkHeader(bool deobfuscate);
    bool finishChunk();
    std::string readString();
    std::wstring readWString();
    void skipChunk();

    template<typename T> T readValue()
    {
        T value; // Short reads leave unread bytes indeterminate in the code.
        m_stream->read(reinterpret_cast<char *>(&value), sizeof(value));
        return value;
    }

    std::list<std::streampos> m_chunkEnds;
    std::istream *m_stream;
    unsigned __int64 m_obfuscationKey;
    std::map<int, std::string> m_chunkNames;
};

}
