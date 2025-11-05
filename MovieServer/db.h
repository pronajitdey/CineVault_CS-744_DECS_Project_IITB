#pragma once
#include <mysql/jdbc.h>
#include <string>
#include <memory>
#include <jsoncons/json.hpp>

using namespace std;

class DBHandler {
  private:
    sql::Driver* driver;
    std::unique_ptr<sql::Connection> con;

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
};