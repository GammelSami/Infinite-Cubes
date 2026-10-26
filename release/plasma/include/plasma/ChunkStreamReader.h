// ChunkStreamReader reads the chunk layer of PLX streams. Each chunk has an identifier, a 32-bit length and a byte
// payload. Name-definition chunks define the identifiers used by the stream.
#pragma once
#include <istream>
#include <list>
#include <map>
#include <string>

namespace plasma {

class ChunkStreamReader {
public:
    explicit ChunkStreamReader(std::istream &stream);

    // Undoes the key-dependent rotation and byte subtraction of sealed bytes.
    void deobfuscate(char *data, int length, const unsigned __int64 &key);
    // Reads the length and pushes the chunk's end position.
    void enterChunk();
    // Reads a chunk identifier and returns its name; an identifier of 0 first defines a new name.
    std::string parseChunkHeader(bool deobfuscate);
    // True (and the chunk is left) at the chunk's end or at end of file.
    bool finishChunk();
    std::string readString();
    std::wstring readWString();
    void skipChunk();

    // Reads a plain value; a short read can leave value indeterminate.
    template<typename T> T readValue()
    {
        T value;
        m_stream->read(reinterpret_cast<char *>(&value), sizeof(value));
        return value;
    }

    std::list<std::streampos> m_chunkEnds; // end positions of the entered chunks
    std::istream *m_stream;
    unsigned __int64 m_obfuscationKey;       // 0 while the file has no (valid) seal
    std::map<int, std::string> m_chunkNames; // identifier -> name
};

}
