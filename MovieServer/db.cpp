#include <iostream>
#include "db.h"

using namespace std;

// Constructor
DBHandler::DBHandler(const string& host, const string& user, 
                     const string& pass, const string& db) 
    : db_host(host), db_user(user), db_pass(pass), db_name(db) {
  driver = get_driver_instance();
  cout << "DBHandler initialized" << endl;
}

// Destructor
DBHandler::~DBHandler() {
  lock_guard<mutex> lock(connections_mutex);
  connections.clear();
  cout << "All database connections closed (auto-cleaned by unique_ptr)" << endl;
  std::cout.flush();
}

// Get or create connection for current thread
sql::Connection* DBHandler::getThreadConnection() {
  thread::id tid = this_thread::get_id();
  
  {
    lock_guard<mutex> lock(connections_mutex);

    if (connections.size() >= MAX_CONNECTIONS) {
      cout << "Connection limit reached (" << MAX_CONNECTIONS 
           << "). Cleaning up all connections..." << endl;
      connections.clear();  // Clear all connections
      cout << "Connections cleaned up. New connections will be created." << endl;
    }

    auto it = connections.find(tid);
    if (it != connections.end() && it->second) {
      return it->second.get();
    }
  }
  
  try {
    sql::Connection* raw_con = driver->connect(db_host, db_user, db_pass);
    raw_con->setSchema(db_name);
    
    unique_ptr<sql::Connection> con(raw_con);
    
    {
      lock_guard<mutex> lock(connections_mutex);
      connections[tid] = std::move(con);  // Transfer ownership
    }
    
    cout << "Created new DB connection for thread " << tid << endl;
    return raw_con;
    
  } catch (sql::SQLException& e) {
    cerr << "Failed to create connection for thread " << tid 
         << ": " << e.what() << endl;
    return nullptr;
  }
}

// Cleanup connection for current thread (after server shutdown)
void DBHandler::cleanupThreadConnection() {
  thread::id tid = this_thread::get_id();
  lock_guard<mutex> lock(connections_mutex);
  
  auto it = connections.find(tid);
  if (it != connections.end()) {
    connections.erase(it);  // unique_ptr automatically deletes connection
    cout << "Cleaned up connection for thread " << tid << endl;
  }
}

// Add a movie in the database
bool DBHandler::addMovie(const string &title, const string &genre, int year, double rating) {
  sql::Connection* con = getThreadConnection();
  if (!con) return false;

  try {
    unique_ptr<sql::PreparedStatement> pstmt
    {con->prepareStatement(
      "INSERT INTO movies (title, genre, release_year, rating) VALUES (?, ?, ?, ?)"
    )};
    pstmt->setString(1, title);
    pstmt->setString(2, genre);
    pstmt->setInt(3, year);
    pstmt->setDouble(4, rating);
    pstmt->executeUpdate();
    return true;
  } catch (sql::SQLException &e) {
    cerr << "AddMovie failed: " << e.what() << endl;
    return false;
  }
}

// List all movies from database
bool DBHandler::listMovies(string &movieListJson) {
  sql::Connection* con = getThreadConnection();
  if (!con) return false;

  try {
    unique_ptr<sql::PreparedStatement> pstmt {
      con->prepareStatement(
        "SELECT * FROM movies ORDER BY id"
      )
    };
    
    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    
    jsoncons::json arr = jsoncons::json::array();
    while (res->next()) {
      jsoncons::json movie;
      movie["id"] = res->getInt("id");
      movie["title"] = string(res->getString("title"));
      movie["genre"] = string(res->getString("genre"));
      movie["release_year"] = res->getInt("release_year");
      movie["rating"] = static_cast<double>(res->getDouble("rating"));
      arr.push_back(movie);
    }

    movieListJson = arr.to_string();
    return true;
  } catch (sql::SQLException &e) {
    cerr << "ListMovies failed: " << e.what() << endl;
    movieListJson = "[]";
    return false;
  }
}

// Find a movie by its title from database
bool DBHandler::searchMovie(const string &title, string &movieJson) {
  sql::Connection* con = getThreadConnection();
  if (!con) return false;

  try {
    unique_ptr<sql::PreparedStatement> pstmt {
      con->prepareStatement(
        "SELECT * FROM movies WHERE LOWER(title) LIKE LOWER(?)"
      )
    };
    string searchPattern = "%" + title + "%";
    pstmt->setString(1, searchPattern);

    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

    jsoncons::json movieArray = jsoncons::json::array();
    while (res->next()) {
      jsoncons::json movieObj;
      movieObj["id"] = res->getInt("id");
      movieObj["title"] = string(res->getString("title"));
      movieObj["genre"] = string(res->getString("genre"));
      movieObj["release_year"] = res->getInt("release_year");
      movieObj["rating"] = static_cast<double>(res->getDouble("rating"));
      movieArray.push_back(movieObj);
    }
    movieJson = movieArray.to_string();
    return true;

  } catch(sql::SQLException& e) {
    cerr << "SearchMovie failed: " << e.what() << endl;
    movieJson = "{}";
    return false;
  }
}

// Update rating of a movie
bool DBHandler::updateRating(int id, double rating, string &title, string &movieJson) {
  sql::Connection* con = getThreadConnection();
  if (!con) return false;

  try {
    unique_ptr<sql::PreparedStatement> pstmt {
      con->prepareStatement(
        "UPDATE movies SET rating = ? WHERE id = ?"
      )
    };
    pstmt->setDouble(1, rating);
    pstmt->setInt(2, id);
    int affected = pstmt->executeUpdate();
    if (affected == 0) {
      cerr << "No movie found with id " << id << endl;
      return false;
    }
    unique_ptr<sql::PreparedStatement> getpstmt {
      con->prepareStatement(
        "SELECT * FROM movies WHERE id = ?"
      )
    };
    getpstmt->setInt(1, id);
    unique_ptr<sql::ResultSet> res(getpstmt->executeQuery());
    if (res->next()) {
      jsoncons::json movie;
      movie["id"] = res->getInt("id");
      movie["title"] = string(res->getString("title"));
      movie["genre"] = string(res->getString("genre"));
      movie["release_year"] = res->getInt("release_year");
      movie["rating"] = static_cast<double>(res->getDouble("rating"));
      
      title = movie["title"].as<string>();
      movieJson = movie.to_string();
    }
    return true;
  } catch (sql::SQLException &e) {
    cerr << "UpdateRating failed: " << e.what() << endl;
    return false;
  }
}

// Delete a movie from database
bool DBHandler::deleteMovie(int id, string &title) {
  sql::Connection* con = getThreadConnection();
  if (!con) return false;
  
  try {
    unique_ptr<sql::PreparedStatement> titlepstmt {
      con->prepareStatement(
        "SELECT title FROM movies WHERE id = ?"
      )
    };
    titlepstmt->setInt(1, id);
    unique_ptr<sql::ResultSet> res(titlepstmt->executeQuery());
    if (res->next()) title = res->getString("title");
    
    unique_ptr<sql::PreparedStatement> pstmt {
      con->prepareStatement(
        "DELETE FROM movies WHERE id = ?"
      )
    };
    pstmt->setInt(1, id);
    int affected = pstmt->executeUpdate();
    if (affected == 0) {
      cerr << "No movie found with id " << id << endl;
      return false;
    }
    return true;
  } catch (sql::SQLException &e) {
    cerr << "DeleteMovie failed: " << e.what() << endl;
    return false;
  }
}
