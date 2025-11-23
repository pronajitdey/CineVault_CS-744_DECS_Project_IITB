#include <iostream>
#include <httplib.h>
#include "db.h"
#include <jsoncons/json.hpp>
#include "cache.h"
#include <algorithm>
#include <cctype>

#define DEFAULT_URI "tcp://127.0.0.1"
#define DB_USER "movieuser"
#define DB_PASS "moviepass"
#define DB_NAME "movie_store"
#define CACHE_CAPACITY 1000

using namespace std;
using namespace jsoncons;

static string to_lower_ascii(const string &s);
static string movie_cache_key(const string &title);

int main() {
  httplib::Server svr;

  const string db_host = DEFAULT_URI;
  const string db_user = DB_USER;
  const string db_pass = DB_PASS;
  const string db_name = DB_NAME;

  DBHandler db(db_host, db_user, db_pass, db_name);
  Cache cache(CACHE_CAPACITY);

  // A test endpoint to check server
  svr.Get("/hi", [](const httplib::Request &, httplib::Response &res) {
    res.set_content("Hello !... This is DECS HTTP server for movie store", "text/plain");
  });

  // Add a movie
  svr.Post("/add-movie", [&](const httplib::Request &req, httplib::Response &res) {
    cout << "Received POST /add-movie request" << endl;

    if (!req.has_param("title") || !req.has_param("genre") || !req.has_param("release-year") || !req.has_param("rating")) {
      res.status = 400;
      res.set_content("Invalid URL", "text/plain");
      return;
    }

    string title = req.get_param_value("title");
    string genre = req.get_param_value("genre");
    int year = stoi(req.get_param_value("release-year"));
    double rating = stod(req.get_param_value("rating"));

    if (db.addMovie(title, genre, year, rating)) {
      json movieJson;
      movieJson["title"] = title;
      movieJson["genre"] = genre;
      movieJson["release_year"] = year;
      movieJson["rating"] = rating;
      cache.put(movie_cache_key(title), movieJson.to_string());
      cache.erase("list_movies");
      res.set_content("Movie added and cached", "text/plain");
    } else {
      res.status = 500;
      res.set_content("Database insertion failed", "text/plain");
    }
  });

  // List all movies
  svr.Get("/list-movies", [&](const httplib::Request &req, httplib::Response &res) {
    cout << "Received GET /list-movies request" << endl;

    string listData;
    if (cache.exists("list_movies")) {
      listData = cache.get("list_movies");
    } else {
      if (db.listMovies(listData)) {
        cache.put("list_movies", listData);
      }
    }
    res.set_content(listData, "application/json");
    // json parsed = json::parse(listData);
    // res.set_content(parsed.to_string(), "application/json");
  });

  // Search a movie
  svr.Get("/search-movie", [&](const httplib::Request &req, httplib::Response &res) {
    cout << "Received GET /search-movie request" << endl;

    if (!req.has_param("title")) {
      res.status = 400;
      res.set_content("Invalid URL", "text/plain");
      return;
    }
    
    string title = req.get_param_value("title");
    string movieData;
    string cacheKey = movie_cache_key(title);
    
    if (cache.exists(cacheKey)) {
      movieData = cache.get(cacheKey);
    } else {
      if (db.searchMovie(title, movieData)) {
        cache.put(cacheKey, movieData);
      }
    }
    res.set_content(movieData, "application/json");
  });

  // Update rating of a movie
  svr.Put("/update-rating", [&](const httplib::Request &req, httplib::Response &res) {
    cout << "Received PUT /udpate-rating request" << endl;

    if (!req.has_param("id") || !req.has_param("rating")) {
      res.status = 400;
      res.set_content("Invalid URL", "text/plain");
      return;
    }

    int id = stoi(req.get_param_value("id"));
    double rating = stod(req.get_param_value("rating"));
    string title;
    string movieJson;

    if (db.updateRating(id, rating, title, movieJson)) {
      if (!title.empty()) {
        string cacheKey = movie_cache_key(title);
        cache.erase(cacheKey);
        cache.put(cacheKey, movieJson);
      }
      cache.erase("list_movies");
      res.set_content("Rating updated", "text/plain");
    } else {
      res.status = 500;
      res.set_content("Updation failed", "text/plain");
    }
  });

  svr.Delete("/delete-movie", [&](const httplib::Request &req, httplib::Response &res) {
    cout << "Received DELETE /delete-movie request" << endl;

    if (!req.has_param("id")) {
      res.status = 400;
      res.set_content("Invalid URL", "text/plain");
      return;
    }

    int id = stoi(req.get_param_value("id"));
    string title;

    if (db.deleteMovie(id, title)) {
      if (!title.empty()) cache.erase(movie_cache_key(title));
      cache.erase("list_movies");
      res.set_content("Movie deleted", "text/plain");
    } else {
      res.status = 500;
      res.set_content("Deletion failed", "text/plain");
    }
  });

  cout << "Server running at http://0.0.0.0:8080" << endl;
  svr.listen("0.0.0.0", 8080);
}

// Convert ASCII string to lowercase (simple, fast)
static string to_lower_ascii(const string &s) {
    string out = s;
    transform(out.begin(), out.end(), out.begin(), ::tolower);
    return out;
}

// Build cache key consistently
static string movie_cache_key(const string &title) {
    return string("movie:") + to_lower_ascii(title);
}
