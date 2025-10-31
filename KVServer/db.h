#pragma once
#include <mysql/jdbc.h>
#include <string>
#include <memory>

using namespace std;

class DBHandler {
  private:
    sql::Driver* driver;
    std::unique_ptr<sql::Connection> con;

  public:
    DBHandler(const std::string& host,
        const std::string& user,
        const std::string& pass,
        const std::string& db);

    ~DBHandler();

    bool create(const std::string& key, const std::string& value);
    bool read(const std::string&key, std::string& value);
    bool remove(const std::string& key);
};