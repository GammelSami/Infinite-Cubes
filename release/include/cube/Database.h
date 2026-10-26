// Database stores named byte blobs in SQLite. This file also declares zlib compression helpers and the data-file byte
// scrambling routines used for database and network buffers.
#pragma once
#include <string>
#include <vector>

struct sqlite3;

namespace cube {

class BytesIO;

// Replaces the buffer's bytes by their zlib-compressed form (compressBytes() into a buffer of size * 1.1 + 12 bytes)
// and rewinds it.
void compressBytes(BytesIO &data);
// Replaces the buffer's bytes by their inflated form and rewinds it; the buffer becomes empty when the data is not a
// zlib stream.
void decompress(BytesIO &data);
// Appends the inflated bytes of data to result, 128000 bytes at a time (the loop of zlib's zpipe.c example). False
// when data is empty or not a complete zlib stream; result keeps what was inflated until then.
bool decompress(BytesIO &data, std::vector<char> &result);
void descramble(BytesIO &data);

class Database {
public:
    Database();
    virtual ~Database();

    void close();
    // "DELETE FROM blobs WHERE key = ?", prepared again while sqlite3_finalize returns SQLITE_SCHEMA (the retry loop
    // of SQLite's blob example, as in read, rename and write). The result is true only if the step returns a row,
    // which a DELETE never does.
    bool erase(const std::string &key);
    // Closes any open database, sqlite3_open(path), then runs "CREATE TABLE blobs(key TEXT PRIMARY KEY, value BLOB);"
    // (it fails harmlessly when the table exists). False if sqlite3_open fails.
    bool open(const std::string &path);
    // "SELECT value FROM blobs WHERE key = ?": copies the blob into value and rewinds it. False when the key is
    // missing.
    bool read(const std::string &key, BytesIO &value);
    // "UPDATE blobs SET key = ? WHERE key = ?" (newKey bound first). False only when the statement cannot be prepared.
    bool rename(const std::string &oldKey, const std::string &newKey);
    // Stores value under key: "UPDATE blobs SET value=? WHERE key=?" when "SELECT 1 FROM blobs WHERE key = ?" finds
    // the key, otherwise "INSERT INTO blobs(key, value) VALUES(?, ?)" (prints "DATABASE WRITE ERROR" when that cannot
    // be prepared).
    bool write(const std::string &key, BytesIO &value);
    bool write(const std::string &key, const char *data, int size);

protected:
    sqlite3 *m_db; // sqlite3_open (open); 0 in the constructor
};

}
