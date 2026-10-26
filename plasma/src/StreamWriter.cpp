#include "plasma/StreamWriter.h"
#include <vector>
#ifdef PLASMA_PORTABLE
#include "Utf16.h"
#endif

namespace plasma {

enum {
    ChunkDefinitionId = 0
};

StreamWriter::StreamWriter(std::ostream &stream, unsigned __int64 key)
    : m_stream(&stream),
      m_nextId(1),
      m_obfuscationKey(key)
{}

// Shared by chunk names and the derived PLX writer's scrambled header.
void StreamWriter::obfuscate(char *data, int length, const unsigned __int64 &key)
{
    const char *keyBytes = reinterpret_cast<const char *>(&key);

    // A zero length divides by zero in the code; preserve that behavior.
    unsigned shift = static_cast<unsigned>(key) % length;
    std::vector<char> sealed(length);
    for (int i = 0; i < length; ++i) {
        unsigned source = (shift + i) % length;
        sealed[i] = keyBytes[static_cast<int>(source) % 8] + data[source];
    }
    for (int i = 0; i < length; ++i)
        data[i] = sealed[i];
}

void StreamWriter::defineChunk(const std::string &name, bool scrambleName)
{
    beginChunkId(ChunkDefinitionId);
    write4(&m_nextId);
    if (scrambleName && m_obfuscationKey) {
        std::vector<char> sealed(name.begin(), name.end());
        int length = sealed.size();
        obfuscate(&sealed[0], length, m_obfuscationKey);
        write4(&length);
        m_stream->write(&sealed[0], length);
    } else {
        writeString(name);
    }
    m_chunkIds[name] = m_nextId;
    ++m_nextId;
    endChunk();
}

void StreamWriter::beginChunk(const std::string &name, bool scrambleName)
{
    if (!m_chunkIds[name])
        defineChunk(name, scrambleName);
    beginChunkId(m_chunkIds[name]);
}

void StreamWriter::beginChunkId(int id)
{
    write4(&id);
    std::streampos start = m_stream->tellp();
    m_chunkStarts.push_back(start);
    int length = 0;
    write4(&length);
}

void StreamWriter::endChunk()
{
    std::streampos end = m_stream->tellp();
    std::streampos start = m_chunkStarts.back();
    m_chunkStarts.pop_back();
    m_stream->seekp(start);
    int length = static_cast<int>(end - start) - 4;
    write4(&length);
    m_stream->seekp(end);
}

void StreamWriter::writeString(const std::string &text)
{
    int length = text.size();
    m_stream->write(reinterpret_cast<const char *>(&length), 4);
    m_stream->write(text.data(), length);
}

void StreamWriter::writeWString(const std::wstring &text)
{
#ifdef PLASMA_PORTABLE
    portable::WriteUtf16(*m_stream, text);
#else
    int length = text.size();
    m_stream->write(reinterpret_cast<const char *>(&length), 4);
    m_stream->write(reinterpret_cast<const char *>(text.data()), 2 * length);
#endif
}

}
