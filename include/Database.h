#pragma once

#include <string>
#include <sqlite3.h>

class Database {
public:
    explicit Database(std::string path = "lms.db");
    ~Database();

    bool connect();
    bool initializeSchema();
    sqlite3* raw() const;

private:
    std::string dbPath_;
    sqlite3* db_;
};
