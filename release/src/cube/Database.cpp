#include "cube/Database.h"

#include <iostream>
#include <string.h>

#include <sqlite3.h>
#include <zlib.h>

#include "cube/BytesIO.h"

namespace cube {

// The scrambling swaps byte i with byte (i + kScrambleOffsets[i % 44]) % size.
static const int kScrambleOffsets[44] = {4242, 9551,  840,   84800, 9242, 9846,  127,   9,     9483, 394,    123,
                                         4834, 32444, 24355, 2433,  17,   34234, 42342, 4243,  14,   184934, 1987,
                                         3094, 1901,  89409, 4813,  37,   143,   3490,  19483, 1343, 432,    84732,
                                         9184, 9612,  1233,  3434,  1839, 2984,  1993,  2984,  4895, 816583, 13};

Database::Database()
    : m_db(nullptr)
{}

Database::~Database()
{
    std::cout << "release db" << std::endl;
    close();
    std::cout << "ok" << std::endl;
}

void Database::close()
{
    if (m_db) {
        sqlite3_close(m_db);
        m_db = nullptr;
    }
}

void compressBytes(BytesIO &data)
{
    data.m_offset = 0;
    std::vector<char> compressed;
    unsigned int size = (unsigned int)data.m_data.size();
    uLongf compressedSize = (uLongf)(size * 1.1) + 12;
    compressed.resize(compressedSize);
    ::compress((Bytef *)&compressed[0], &compressedSize, (const Bytef *)&data.m_data[0], size);
    compressed.resize(compressedSize);
    data.m_data = compressed;
}

void decompress(BytesIO &data)
{
    data.m_offset = 0;
    std::vector<char> decompressed;
    decompress(data, decompressed);
    data.m_data = decompressed;
}

bool decompress(BytesIO &data, std::vector<char> &result)
{
    const int kChunkSize = 128000;
    unsigned char chunk[kChunkSize];
    z_stream stream;

    if (data.m_data.empty()) {
        return false;
    }
    stream.zalloc = Z_NULL;
    stream.zfree = Z_NULL;
    stream.opaque = Z_NULL;
    stream.avail_in = 0;
    stream.next_in = Z_NULL;
    if (inflateInit(&stream) != Z_OK) {
        return false;
    }
    stream.avail_in = (uInt)data.m_data.size();
    stream.next_in = (Bytef *)&data.m_data[0];
    int status;
    do {
        stream.avail_out = kChunkSize;
        stream.next_out = chunk;
        status = inflate(&stream, Z_NO_FLUSH);
        switch (status) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
            inflateEnd(&stream);
            return false;
        }
        size_t have = kChunkSize - stream.avail_out;
        unsigned int size = (unsigned int)result.size();
        result.resize(result.size() + have);
        memcpy(&result[size], chunk, have);
    } while (status != Z_STREAM_END);
    inflateEnd(&stream);
    return status == Z_STREAM_END;
}

void descramble(BytesIO &data)
{
    for (int i = data.size() - 1; 0 <= i; i--) {
        size_t other = (i + kScrambleOffsets[i % 44]) % data.m_data.size();
        std::swap(data.m_data[i], data.m_data[other]);
    }
    for (int i = 0; i < data.size(); i++) {
        data.m_data[i] = ~data.m_data[i];
    }
}

bool Database::erase(const std::string &key)
{
    sqlite3_stmt *statement;
    int result;
    do {
        result = sqlite3_prepare(m_db, "DELETE FROM blobs WHERE key = ?", -1, &statement, nullptr);
        if (result != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(statement, 1, key.c_str(), -1, SQLITE_STATIC);
        result = sqlite3_step(statement);
        if (result == SQLITE_ROW) {
            sqlite3_finalize(statement);
            return true;
        }
        result = sqlite3_finalize(statement);
    } while (result == SQLITE_SCHEMA);
    return false;
}

bool Database::open(const std::string &path)
{
    close();
    if (sqlite3_open(path.c_str(), &m_db) != SQLITE_OK) {
        return false;
    }
    sqlite3_exec(m_db, "CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB);", nullptr, nullptr, nullptr);
    return true;
}

bool Database::read(const std::string &key, BytesIO &value)
{
    sqlite3_stmt *statement;
    int result;
    do {
        result = sqlite3_prepare(m_db, "SELECT value FROM blobs WHERE key = ?", -1, &statement, nullptr);
        if (result != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(statement, 1, key.c_str(), -1, SQLITE_STATIC);
        result = sqlite3_step(statement);
        if (result == SQLITE_ROW) {
            int size = sqlite3_column_bytes(statement, 0);
            unsigned char *blob = new unsigned char[size];
            memcpy(blob, sqlite3_column_blob(statement, 0), size);
            sqlite3_finalize(statement);
            value.m_offset = 0;
            value.m_data.resize(size);
            memcpy(&value.m_data[0], blob, size);
            delete[] blob;
            return true;
        }
        result = sqlite3_finalize(statement);
    } while (result == SQLITE_SCHEMA);
    return false;
}

bool Database::rename(const std::string &oldKey, const std::string &newKey)
{
    sqlite3_stmt *statement;
    int result;
    do {
        result = sqlite3_prepare(m_db, "UPDATE blobs SET key = ? WHERE key = ?", -1, &statement, nullptr);
        if (result != SQLITE_OK) {
            return false;
        }
        sqlite3_bind_text(statement, 1, newKey.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(statement, 2, oldKey.c_str(), -1, SQLITE_STATIC);
        result = sqlite3_step(statement);
        if (result == SQLITE_ROW) {
            sqlite3_finalize(statement);
            return true;
        }
        result = sqlite3_finalize(statement);
    } while (result == SQLITE_SCHEMA);
    return true;
}

bool Database::write(const std::string &key, BytesIO &value)
{
    size_t size = value.m_data.size();
    return write(key, value.m_data.empty() ? nullptr : &value.m_data[0], (int)size);
}

bool Database::write(const std::string &key, const char *data, int size)
{
    sqlite3_stmt *statement;
    if (sqlite3_prepare(m_db, "SELECT 1 FROM blobs WHERE key = ?", -1, &statement, nullptr) != SQLITE_OK) {
        return false;
    }
    if (sqlite3_bind_text(statement, 1, key.c_str(), -1, SQLITE_STATIC) != SQLITE_OK) {
        return false;
    }
    bool exists = sqlite3_step(statement) == SQLITE_ROW;
    sqlite3_finalize(statement);

    int result;
    if (exists) {
        sqlite3_stmt *update;
        do {
            result = sqlite3_prepare(m_db, "UPDATE blobs SET value=? WHERE key=?", -1, &update, nullptr);
            if (result != SQLITE_OK) {
                return false;
            }
            sqlite3_bind_blob(update, 1, data, size, SQLITE_STATIC);
            sqlite3_bind_text(update, 2, key.c_str(), -1, SQLITE_STATIC);
            sqlite3_step(update);
            result = sqlite3_finalize(update);
        } while (result == SQLITE_SCHEMA);
    } else {
        sqlite3_stmt *insert;
        do {
            result = sqlite3_prepare(m_db, "INSERT INTO blobs(key, value) VALUES(?, ?)", -1, &insert, nullptr);
            if (result != SQLITE_OK) {
                std::cout << "DATABASE WRITE ERROR" << std::endl;
                return false;
            }
            sqlite3_bind_text(insert, 1, key.c_str(), -1, SQLITE_STATIC);
            sqlite3_bind_blob(insert, 2, data, size, SQLITE_STATIC);
            sqlite3_step(insert);
            result = sqlite3_finalize(insert);
        } while (result == SQLITE_SCHEMA);
    }
    return true;
}

}
