#pragma once
#include <string>

struct sqlite3;

namespace cube {
class BytesIO;

// SQLite blobs keyed by text.
class Database {
public:
    Database();
    virtual ~Database();
    void close();
    bool erase(const std::string &key);
    bool isOpen();
    bool open(const std::string &path);
    // Allocates a copy with new[]; the caller owns it.
    bool read(const std::string &key, unsigned char *&data, int &size);
    bool read(const std::string &key, BytesIO &value);
    bool rename(const std::string &oldKey, const std::string &newKey);
    bool write(const std::string &key, BytesIO &value);
    bool write(const std::string &key, const char *data, int size);

private:
    sqlite3 *m_db;
};
}
