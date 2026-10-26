#include "plasma/ChunkStreamReader.h"
#include "plasma/PLXStreamReader.h"
#include <vector>
#ifdef PLASMA_PORTABLE
#include "Utf16.h"
#endif

namespace plasma {

ChunkStreamReader::ChunkStreamReader(std::istream &stream)
    : m_stream(&stream),
      m_obfuscationKey(0)
{}

// Undoes the key-dependent rotation and byte addition of a sealed name.
void deobfuscate(char *data, int length, const unsigned __int64 &key)
{
    const char *keyBytes = reinterpret_cast<const char *>(&key);

    // A zero length divides by zero in the code; preserve that behavior.
    unsigned shift = static_cast<unsigned>(key) % length;
    std::vector<char> clear(length);
    for (int i = 0; i < length; ++i) {
        unsigned destination = (shift + i) % length;
        clear[destination] = data[i] - keyBytes[destination % 8];
    }
    for (int i = 0; i < length; ++i)
        data[i] = clear[i];
}

void ChunkStreamReader::skipChunk()
{
    int length; // The code uses the indeterminate value after a short read.
    m_stream->read(reinterpret_cast<char *>(&length), 4);
    m_stream->seekg(m_stream->tellg() + static_cast<std::streamoff>(length));
}

void ChunkStreamReader::enterChunk()
{
    int length; // The code uses the indeterminate value after a short read.
    m_stream->read(reinterpret_cast<char *>(&length), 4);
    m_chunkEnds.push_back(m_stream->tellg() + static_cast<std::streamoff>(length));
}

bool ChunkStreamReader::finishChunk()
{
    if (!m_stream->eof() && m_stream->tellg() != m_chunkEnds.back())
        return false;
    m_chunkEnds.pop_back();
    return true;
}

// An identifier of 0 introduces a name definition (a chunk holding the new identifier and its name, sealed when
// the file has a key); the identifier after it names the chunk.
std::string ChunkStreamReader::parseChunkHeader()
{
    int id = readInt();
#ifdef PLASMA_PORTABLE

    // EOF is checked here before an indeterminate ID can be treated as a name.
    if (!*m_stream)
        return std::string();
#endif
    if (id == 0) {
        enterChunk();
        int newId = readInt();
        std::string name;
        if (m_obfuscationKey) {
            int length = readInt();
            std::vector<char> bytes(length);
            m_stream->read(&bytes[0], length);
            deobfuscate(&bytes[0], length, m_obfuscationKey);
            name = std::string(bytes.begin(), bytes.end());
        } else {
            name = readString();
        }
        m_chunkNames[newId] = name;
        finishChunk();
        id = readInt();
    }
    return m_chunkNames[id];
}

std::string ChunkStreamReader::readString()
{
    int length = readInt();
    if (!length)
        return std::string("");
    char *bytes = new char[length + 1];
    bytes[length] = 0;
    m_stream->read(bytes, length);
    std::string result(bytes);
    delete[] bytes;
    return result;
}

std::wstring ChunkStreamReader::readWString()
{
#ifdef PLASMA_PORTABLE
    return portable::ReadUtf16(*m_stream, readInt());
#else
    int length = readInt();
    if (!length)
        return std::wstring(L"");
    wchar_t *characters = new wchar_t[length + 1];
    characters[length] = 0;
    m_stream->read(reinterpret_cast<char *>(characters), 2 * length);
    std::wstring result(characters);
    delete[] characters;
    return result;
#endif
}

}
