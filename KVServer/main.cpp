#include <iostream>
#include <httplib.h>
#include "db.h"
#include <jsoncons/json.hpp>
#include "cache.h"

#define DEFAULT_URI "tcp://127.0.0.1"
#define DB_USER "kvdbuser"
#define DB_PASS "kvdbpass"
#define DB_NAME "kvstore"
#define CACHE_CAPACITY 1000

using namespace std;
using namespace jsoncons;

int main() {
  httplib::Server svr;

  const string db_host = DEFAULT_URI;
  const string db_user = DB_USER;
  const string db_pass = DB_PASS;
  const string db_name = DB_NAME;

  DBHandler db(db_host, db_user, db_pass, db_name);
  Cache cache(CACHE_CAPACITY);

  // a test endpoint to check server
  svr.Get("/hi", [](const httplib::Request &, httplib::Response &res) {
    res.set_content("Hello !... This is DECS HTTP server for KV store", "text/plain");
  });

  // /create endpoint
  svr.Post("/create", [&](const httplib::Request &req, httplib::Response &res) {
    cout << "Received POST /create request" << endl;
    
    try {
      json j = json::parse(req.body);
      string key = j["key"].as<string>();
      string value = j["value"].as<string>();

      if (db.create(key, value)) {
        cache.insert(key, value);
        res.set_content("Inserted/Updated successfully", "text/plain");
      } else {
        res.status = 500;
        res.set_content("Database insertion failed", "text/plain");
      }

    } catch (const exception &e) {
      res.status = 400;
      res.set_content(string("Invalid JSON: ") + e.what(), "text/plain");
    }
  });

  // /read endpoint
  svr.Post("/read", [&](const httplib::Request &req, httplib::Response &res){
    cout << "Received POST /read request" << endl;

    try {
      json j = json::parse(req.body);
      string key = j["key"].as<string>();
      string value;

      if (cache.read(key, value)) {
        cout << "Cache hit for key: " << key << endl;
        json response;
        response["key"] = key;
        response["value"] = value;
        res.set_content(response.to_string(), "application/json");
        return;
      }

      cout << "Cache miss for key: " << key << endl;
      if (db.read(key, value)) {
        cache.insert(key, value);
        json response;
        response["key"] = key;
        response["value"] = value;
        res.set_content(response.to_string(), "application/json");
      } else {
        res.status = 404;
        res.set_content(R"({"error": "Key not found"})", "application/json");
      }

    } catch (const exception &e) {
      res.status = 400;
      res.set_content(string("Invalid JSON: ") + e.what(), "text/plain");
    }
  });

  // /delete endpoint
  svr.Post("/delete", [&](const httplib::Request &req, httplib::Response &res) {
    cout << "Received POST /delete request" << endl;

    try {
      json j = json::parse(req.body);
      string key = j["key"].as<string>();

      if (db.remove(key)) {
        cache.remove(key);
        res.set_content("Deleted successfully", "text/plain");
      } else {
        res.status = 404;
        res.set_content(R"({"error": "Key not found"})", "application/json");
      }

    } catch (const exception &e) {
      res.status = 400;
      res.set_content(string("Invalid JSON: ") + e.what(), "text/plain");
    }
  });

  cout << "Server running at http://localhost:8080" << endl;
  svr.listen("0.0.0.0", 8080);
}