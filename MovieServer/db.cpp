#include <iostream>
#include "db.h"

using namespace std;

// Constructor
DBHandler::DBHandler(const string& host,
    const string& user,
    const string& pass,
    const string& db) {
  
  try {
    driver = sql::mysql::get_driver_instance();
    con.reset(driver->connect(host, user, pass));
    con->setSchema(db);
    cout << "Connected to MySQL database: " << db << endl;
  } catch(sql::SQLException &e) {
    cerr << "#ERR: " << e.what()
         << " (MySQL error code: " << e.getErrorCode()
         << ", SQLState: " << e.getSQLState() << " )" << endl;
  }
}

// Destructor
DBHandler::~DBHandler() = default;

// Add a movie in the database
bool DBHandler::addMovie(const string &title, const string &genre, int year, double rating) {
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
  try {
    unique_ptr<sql::PreparedStatement> pstmt {
      con->prepareStatement(
        "SELECT * FROM movies WHERE title = ?"
      )
    };
    pstmt->setString(1, title);

    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());

    if (res->next()) {
      jsoncons::json movie;
      movie["id"] = res->getInt("id");
      movie["title"] = string(res->getString("title"));
      movie["genre"] = string(res->getString("genre"));
      movie["release_year"] = res->getInt("release_year");
      movie["rating"] = static_cast<double>(res->getDouble("rating"));
      
      movieJson = movie.to_string();
      return true;
    } else {
      movieJson = "{}";
      return false;
    }
  } catch(sql::SQLException& e) {
    cerr << "SearchMovie failed: " << e.what() << endl;
    movieJson = "{}";
    return false;
  }
}

// Update rating of a movie
bool DBHandler::updateRating(int id, double rating, string &title, string &movieJson) {
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
