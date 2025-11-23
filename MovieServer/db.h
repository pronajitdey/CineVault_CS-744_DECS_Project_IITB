#pragma once
#include <mysql/jdbc.h>
#include <string>
#include <memory>
#include <jsoncons/json.hpp>
#include <mutex>
#include <thread>
#include <unordered_map>

using namespace std;

class DBHandler {
  private:
    std::string db_host;
    std::string db_user;
    std::string db_pass;
    std::string db_name;
    sql::Driver* driver;

    std::unordered_map<std::thread::id, std::unique_ptr<sql::Connection>> connections;
    std::mutex connections_mutex;

    static const int MAX_CONNECTIONS = 100;
    
    sql::Connection* getThreadConnection();

  public:
    DBHandler(const string& host,
        const string& user,
        const string& pass,
        const string& db);

    ~DBHandler();

    bool addMovie(const string &title, const string &genre, int year, double rating);
    bool listMovies(string &movieJson);
    bool searchMovie(const string &title, string &movieJson);
    bool updateRating(int id, double rating, string &title, string &movieJson);
    bool deleteMovie(int id, string &title);

    void cleanupThreadConnection();
};