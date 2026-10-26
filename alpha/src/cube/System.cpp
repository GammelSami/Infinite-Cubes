// System supplies script callbacks for file access, MD5 hashing and machine information. The MD5 implementation follows RFC 1321.
#include <stdio.h>
#include <string.h>
#include <fstream>
#include <sstream>
#include "abstr/Method.h"
#include "cube/System.h"
#include "cube/Scripting.h"

#if CUBE_SCRIPTING

namespace cube {

typedef unsigned int UINT4;

// RFC 1321 MD5: the context, the binary digest and its hexadecimal form.
class MD5 {
public:
    MD5() { Init(); }
    char *digestFile(char *filename);
    char *digestMemory(unsigned char *memchunk, int len)
    {
        Init();
        Update(memchunk, len);
        Final();
        return digestChars;
    }
    char *digestString(char *string)
    {
        Init();
        Update((unsigned char *)string, strlen(string));
        Final();
        return digestChars;
    }

private:
    void Init()
    {
        context.count[0] = context.count[1] = 0;
        context.state[0] = 0x67452301;
        context.state[1] = 0xefcdab89;
        context.state[2] = 0x98badcfe;
        context.state[3] = 0x10325476;
    }
    void Update(unsigned char *input, unsigned int inputLen);
    void Final();
    static void Transform(UINT4 state[4], unsigned char block[64]);
    static void Encode(unsigned char *output, UINT4 *input, unsigned int len)
    {
        unsigned int i, j;
        for (i = 0, j = 0; j < len; i++, j += 4) {
            output[j] = (unsigned char)(input[i] & 0xff);
            output[j + 1] = (unsigned char)((input[i] >> 8) & 0xff);
            output[j + 2] = (unsigned char)((input[i] >> 16) & 0xff);
            output[j + 3] = (unsigned char)((input[i] >> 24) & 0xff);
        }
    }
    static void Decode(UINT4 *output, unsigned char *input, unsigned int len)
    {
        unsigned int i, j;
        for (i = 0, j = 0; j < len; i++, j += 4)
            output[i] = ((UINT4)input[j]) | (((UINT4)input[j + 1]) << 8) | (((UINT4)input[j + 2]) << 16) |
                        (((UINT4)input[j + 3]) << 24);
    }
    void writeToString()
    {
        int pos;
        for (pos = 0; pos < 16; pos++)
            sprintf(digestChars + (pos * 2), "%02x", digestRaw[pos]);
    }

    struct Context {
        UINT4 state[4];           // ABCD
        UINT4 count[2];           // number of bits, modulo 2^64
        unsigned char buffer[64]; // input buffer
    } context;
    unsigned char digestRaw[16];
    char digestChars[33];
};

static unsigned char PADDING[64] = {0x80};

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define FF(a, b, c, d, x, s, ac)                                                                                       \
    {                                                                                                                  \
        (a) += F((b), (c), (d)) + (x) + (UINT4)(ac);                                                                   \
        (a) = ROTATE_LEFT((a), (s));                                                                                   \
        (a) += (b);                                                                                                    \
    }
#define GG(a, b, c, d, x, s, ac)                                                                                       \
    {                                                                                                                  \
        (a) += G((b), (c), (d)) + (x) + (UINT4)(ac);                                                                   \
        (a) = ROTATE_LEFT((a), (s));                                                                                   \
        (a) += (b);                                                                                                    \
    }
#define HH(a, b, c, d, x, s, ac)                                                                                       \
    {                                                                                                                  \
        (a) += H((b), (c), (d)) + (x) + (UINT4)(ac);                                                                   \
        (a) = ROTATE_LEFT((a), (s));                                                                                   \
        (a) += (b);                                                                                                    \
    }
#define II(a, b, c, d, x, s, ac)                                                                                       \
    {                                                                                                                  \
        (a) += I((b), (c), (d)) + (x) + (UINT4)(ac);                                                                   \
        (a) = ROTATE_LEFT((a), (s));                                                                                   \
        (a) += (b);                                                                                                    \
    }

System::System(bool test)
    : m_test(test)
{
    m_machine.loadProgramFromFile("interface.plx");
    m_machine.addMethod("button:click", this, &System::newBuffer);
    m_machine.addMethod("button:hover", this, &System::loadBuffer);
    m_machine.addMethod("edit:enter", this, &System::bufferFromString);
    m_machine.addMethod("edit:leave", this, &System::deleteBuffer);
    m_machine.addMethod("scroll:up", this, &System::setBufferByte);
    m_machine.addMethod("scroll:down", this, &System::bufferByte);
    m_machine.addMethod("scroll:click", this, &System::bufferSize);
    m_machine.addMethod("window:resize", this, &System::resizeBuffer);
    m_machine.addMethod("window:maximize", this, &System::appendInt);
    m_machine.addMethod("window:close", this, &System::readAdapters);
    m_machine.addMethod("window:minimize", this, &System::firstAdapter);
    m_machine.addMethod("screen:start", this, &System::nextAdapter);
    m_machine.addMethod("screen:options", this, &System::adapterType);
    m_machine.addMethod("screen:system", this, &System::adapterAddressByte);
    m_machine.addMethod("screen:save", this, &System::fileHash);
    m_machine.addMethod("checkbox:mark", this, &System::stringHash);
    m_machine.addMethod("checkbox:unmark", this, &System::bufferHash);
    m_machine.addMethod("static:enable", this, &System::openFile);
    m_machine.addMethod("static:disable", this, &System::closeFile);
    m_machine.addMethod("static:1", this, &System::creationTimeHigh);
    m_machine.addMethod("static:2", this, &System::creationTimeLow);
    m_machine.addMethod("static:3", this, &System::accessTimeHigh);
    m_machine.addMethod("static:4", this, &System::accessTimeLow);
    m_machine.addMethod("static:5", this, &System::writeTimeHigh);
    m_machine.addMethod("static:7", this, &System::writeTimeLow);
    m_machine.addMethod("widget:enter", this, &System::modulePath);
    m_machine.addMethod("widget:leave", this, &System::volumeSerial);
    m_machine.addMethod("widget:show", this, &System::concatenate);
    m_machine.addMethod("widget:hide", this, &System::setTexts);
    m_machine.addMethod("widget:enable", this, &System::setText);
    m_machine.addMethod("widget:close", this, &System::bufferString);
    m_machine.addMethod("widget:open", this, &System::equal);
    m_machine.callFunction("test", test);
}

void MD5::Final()
{
    unsigned char bits[8];
    unsigned int index, padLen;

    Encode(bits, context.count, 8);

    index = (unsigned int)((context.count[0] >> 3) & 0x3f);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    Update(PADDING, padLen);

    Update(bits, 8);

    Encode(digestRaw, context.state, 16);

    memset(&context, 0, sizeof(context));

    writeToString();
}

void MD5::Transform(UINT4 state[4], unsigned char block[64])
{
    UINT4 a = state[0], b = state[1], c = state[2], d = state[3], x[16];

    Decode(x, block, 64);

    // Round 1
    FF(a, b, c, d, x[0], S11, 0xd76aa478);
    FF(d, a, b, c, x[1], S12, 0xe8c7b756);
    FF(c, d, a, b, x[2], S13, 0x242070db);
    FF(b, c, d, a, x[3], S14, 0xc1bdceee);
    FF(a, b, c, d, x[4], S11, 0xf57c0faf);
    FF(d, a, b, c, x[5], S12, 0x4787c62a);
    FF(c, d, a, b, x[6], S13, 0xa8304613);
    FF(b, c, d, a, x[7], S14, 0xfd469501);
    FF(a, b, c, d, x[8], S11, 0x698098d8);
    FF(d, a, b, c, x[9], S12, 0x8b44f7af);
    FF(c, d, a, b, x[10], S13, 0xffff5bb1);
    FF(b, c, d, a, x[11], S14, 0x895cd7be);
    FF(a, b, c, d, x[12], S11, 0x6b901122);
    FF(d, a, b, c, x[13], S12, 0xfd987193);
    FF(c, d, a, b, x[14], S13, 0xa679438e);
    FF(b, c, d, a, x[15], S14, 0x49b40821);

    // Round 2
    GG(a, b, c, d, x[1], S21, 0xf61e2562);
    GG(d, a, b, c, x[6], S22, 0xc040b340);
    GG(c, d, a, b, x[11], S23, 0x265e5a51);
    GG(b, c, d, a, x[0], S24, 0xe9b6c7aa);
    GG(a, b, c, d, x[5], S21, 0xd62f105d);
    GG(d, a, b, c, x[10], S22, 0x2441453);
    GG(c, d, a, b, x[15], S23, 0xd8a1e681);
    GG(b, c, d, a, x[4], S24, 0xe7d3fbc8);
    GG(a, b, c, d, x[9], S21, 0x21e1cde6);
    GG(d, a, b, c, x[14], S22, 0xc33707d6);
    GG(c, d, a, b, x[3], S23, 0xf4d50d87);
    GG(b, c, d, a, x[8], S24, 0x455a14ed);
    GG(a, b, c, d, x[13], S21, 0xa9e3e905);
    GG(d, a, b, c, x[2], S22, 0xfcefa3f8);
    GG(c, d, a, b, x[7], S23, 0x676f02d9);
    GG(b, c, d, a, x[12], S24, 0x8d2a4c8a);

    // Round 3
    HH(a, b, c, d, x[5], S31, 0xfffa3942);
    HH(d, a, b, c, x[8], S32, 0x8771f681);
    HH(c, d, a, b, x[11], S33, 0x6d9d6122);
    HH(b, c, d, a, x[14], S34, 0xfde5380c);
    HH(a, b, c, d, x[1], S31, 0xa4beea44);
    HH(d, a, b, c, x[4], S32, 0x4bdecfa9);
    HH(c, d, a, b, x[7], S33, 0xf6bb4b60);
    HH(b, c, d, a, x[10], S34, 0xbebfbc70);
    HH(a, b, c, d, x[13], S31, 0x289b7ec6);
    HH(d, a, b, c, x[0], S32, 0xeaa127fa);
    HH(c, d, a, b, x[3], S33, 0xd4ef3085);
    HH(b, c, d, a, x[6], S34, 0x4881d05);
    HH(a, b, c, d, x[9], S31, 0xd9d4d039);
    HH(d, a, b, c, x[12], S32, 0xe6db99e5);
    HH(c, d, a, b, x[15], S33, 0x1fa27cf8);
    HH(b, c, d, a, x[2], S34, 0xc4ac5665);

    // Round 4
    II(a, b, c, d, x[0], S41, 0xf4292244);
    II(d, a, b, c, x[7], S42, 0x432aff97);
    II(c, d, a, b, x[14], S43, 0xab9423a7);
    II(b, c, d, a, x[5], S44, 0xfc93a039);
    II(a, b, c, d, x[12], S41, 0x655b59c3);
    II(d, a, b, c, x[3], S42, 0x8f0ccc92);
    II(c, d, a, b, x[10], S43, 0xffeff47d);
    II(b, c, d, a, x[1], S44, 0x85845dd1);
    II(a, b, c, d, x[8], S41, 0x6fa87e4f);
    II(d, a, b, c, x[15], S42, 0xfe2ce6e0);
    II(c, d, a, b, x[6], S43, 0xa3014314);
    II(b, c, d, a, x[13], S44, 0x4e0811a1);
    II(a, b, c, d, x[4], S41, 0xf7537e82);
    II(d, a, b, c, x[11], S42, 0xbd3af235);
    II(c, d, a, b, x[2], S43, 0x2ad7d2bb);
    II(b, c, d, a, x[9], S44, 0xeb86d391);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;

    memset((unsigned char *)x, 0, sizeof(x));
}

#undef S11
#undef S12
#undef S13
#undef S14
#undef S21
#undef S22
#undef S23
#undef S24
#undef S31
#undef S32
#undef S33
#undef S34
#undef S41
#undef S42
#undef S43
#undef S44
#undef F
#undef G
#undef H
#undef I
#undef ROTATE_LEFT
#undef FF
#undef GG
#undef HH
#undef II

void MD5::Update(unsigned char *input, unsigned int inputLen)
{
    unsigned int i, index, partLen;

    index = (unsigned int)((context.count[0] >> 3) & 0x3F);

    if ((context.count[0] += ((UINT4)inputLen << 3)) < ((UINT4)inputLen << 3))
        context.count[1]++;
    context.count[1] += ((UINT4)inputLen >> 29);

    partLen = 64 - index;

    if (inputLen >= partLen) {
        memcpy(&context.buffer[index], input, partLen);
        Transform(context.state, context.buffer);

        for (i = partLen; i + 63 < inputLen; i += 64)
            Transform(context.state, &input[i]);

        index = 0;
    } else {
        i = 0;
    }

    memcpy(&context.buffer[index], &input[i], inputLen - i);
}

void System::appendInt(int buffer, int value)
{
    std::vector<char> &data = *(std::vector<char> *)buffer;
    size_t size = data.size();
    data.resize(size + sizeof(int));
    *(int *)&data[size] = value;
}

int System::bufferSize(int buffer)
{
    return ((std::vector<char> *)buffer)->size();
}

std::string System::bufferString(int buffer)
{
    std::vector<char> &data = *(std::vector<char> *)buffer;
    return std::string(data.begin(), data.end());
}

void System::closeFile(int file)
{
    CloseHandle((HANDLE)file);
}

std::string System::concatenate(std::string first, std::string second)
{
    return first + second;
}

void System::deleteBuffer(int buffer)
{
    std::vector<char> *data = (std::vector<char> *)buffer;
    m_buffers.remove(data);
    delete data;
}

std::string System::bufferHash(int buffer)
{
    std::vector<char> &data = *(std::vector<char> *)buffer;
    MD5 md5;
    if (data.empty())
        return "";
    return md5.digestMemory((unsigned char *)&data[0], data.size());
}

char *MD5::digestFile(char *filename)
{
    Init();

    FILE *file;
    int len;
    unsigned char buffer[1024]; // mddriver.c's MDFile buffer

    if ((file = fopen(filename, "rb")) == NULL) {
        printf("%s can't be opened\n", filename);
    } else {
        while (len = fread(buffer, 1, 1024, file))
            Update(buffer, len);
        Final();

        fclose(file);
    }

    return digestChars;
}

std::string System::fileHash(std::string path)
{
    MD5 md5;
    return md5.digestFile((char *)path.c_str());
}

std::string System::stringHash(std::string text)
{
    MD5 md5;
    return md5.digestMemory((unsigned char *)text.c_str(), text.size());
}

int System::adapterAddressByte(int adapter, int index)
{
    return ((IP_ADAPTER_INFO *)adapter)->Address[index];
}

int System::adapterType(int adapter)
{
    return ((IP_ADAPTER_INFO *)adapter)->Type;
}

int System::readAdapters()
{
    ULONG size = sizeof(m_adapters);
    return GetAdaptersInfo(m_adapters, &size);
}

int System::bufferByte(int buffer, int index)
{
    return (unsigned char)(*(std::vector<char> *)buffer)[index];
}

int System::creationTimeHigh(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, &time, NULL, NULL);
    return time.dwHighDateTime;
}

int System::creationTimeLow(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, &time, NULL, NULL);
    return time.dwLowDateTime;
}

int System::accessTimeHigh(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, NULL, &time, NULL);
    return time.dwHighDateTime;
}

int System::accessTimeLow(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, NULL, &time, NULL);
    return time.dwLowDateTime;
}

int System::writeTimeHigh(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, NULL, NULL, &time);
    return time.dwHighDateTime;
}

int System::writeTimeLow(int file)
{
    FILETIME time;
    GetFileTime((HANDLE)file, NULL, NULL, &time);
    return time.dwLowDateTime;
}

int System::firstAdapter()
{
    return (int)m_adapters;
}

int System::nextAdapter(int adapter)
{
    return (int)((IP_ADAPTER_INFO *)adapter)->Next;
}

std::string System::modulePath()
{
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH - 1);
    return path;
}

int System::newBuffer()
{
    m_buffers.push_back(new std::vector<char>);
    return (int)m_buffers.back();
}

int System::loadBuffer(std::string path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file.good()) {
        m_buffers.push_back(new std::vector<char>(0));
    } else {
        file.seekg(0, std::ios::end);
        std::streamoff size = file.tellg();
        file.seekg(0, std::ios::beg);
        m_buffers.push_back(new std::vector<char>((size_t)size));
        if (size != 0)
            file.read(&(*m_buffers.back())[0], size);
    }
    return (int)m_buffers.back();
}

int System::openFile(std::string path)
{
    return (int)CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                            NULL);
}

void System::resizeBuffer(int buffer, int size)
{
    ((std::vector<char> *)buffer)->resize(size);
}

void System::setBufferByte(int buffer, int index, int value)
{
    (*(std::vector<char> *)buffer)[index] = (char)value;
}

void System::setText(std::string text)
{
    m_text = text;
}

void System::setTexts(std::string first, std::string second)
{
    m_firstText = first;
    m_secondText = second;
}

int System::equal(std::string first, std::string second)
{
    return first == second;
}

int System::bufferFromString(std::string text)
{
    size_t size = text.size();
    m_buffers.push_back(new std::vector<char>(size));
    if (size != 0)
        memcpy(&(*m_buffers.back())[0], text.c_str(), size);
    return (int)m_buffers.back();
}
// The volume's name and its file system's name, each in a buffer of VolumeNameCapacity characters
// (anonymous: this file's own).
namespace {
enum {
    VolumeNameCapacity = 256
};
}

std::string System::volumeSerial()
{
    char volumeName[VolumeNameCapacity];
    char fileSystemName[VolumeNameCapacity];
    DWORD serialNumber;
    DWORD maximumComponentLength;
    DWORD fileSystemFlags;
    BOOL found = GetVolumeInformationA(NULL, volumeName, VolumeNameCapacity - 1, &serialNumber, &maximumComponentLength,
                                       &fileSystemFlags, fileSystemName, VolumeNameCapacity - 1);
    std::stringstream stream;
    if (found)
        stream << serialNumber;
    if (stream.str().empty())
        stream << "default";
    return stream.str();
}

}

#endif // CUBE_SCRIPTING
