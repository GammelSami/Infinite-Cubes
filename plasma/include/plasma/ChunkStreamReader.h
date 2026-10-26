#pragma once
#include <istream>
#include <list>
#include <map>
#include <string>

namespace plasma {

// ChunkStreamReader.cpp. The list contains the end positions of entered chunks. The map translates numeric chunk
// identifiers to names.
class ChunkStreamReader {
public:
    explicit ChunkStreamReader(std::istream &stream);
    void skipChunk();
    void enterChunk();
    bool finishChunk();
    std::string parseChunkHeader();
    std::string readString();
    std::wstring readWString();
    int readInt();
    float readFloat();

    std::list<std::streampos> m_chunkEnds;
    std::istream *m_stream;
    unsigned __int64 m_obfuscationKey;
    std::map<int, std::string> m_chunkNames;
};

}
