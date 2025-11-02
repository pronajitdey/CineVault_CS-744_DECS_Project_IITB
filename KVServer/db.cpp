#include <iostream>
#include "db.h"

using namespace std;

// constructor
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

// destructor
DBHandler::~DBHandler() = default;

// insert or update key in the database
bool DBHandler::create(const string& key, const string& value) {
  try {
    unique_ptr<sql::PreparedStatement> pstmt{con->prepareStatement(
      "INSERT INTO kv_pairs (k, v) VALUES (?, ?) ON DUPLICATE KEY UPDATE v = VALUES(v)"
    )};
    pstmt->setString(1, key);
    pstmt->setString(2, value);
    pstmt->executeUpdate();
    return true;
  } catch (sql::SQLException &e) {
    cerr << "Upsert error: " << e.what() << endl;
    return false;
  }
}

// read value of a key from database
bool DBHandler::read(const string& key, string& value) {
  try {
    unique_ptr<sql::PreparedStatement> pstmt {
      con->prepareStatement(
        "SELECT v FROM kv_pairs WHERE k = ?"
      )
    };
    pstmt->setString(1, key);

    unique_ptr<sql::ResultSet> res(pstmt->executeQuery());
    if (res->next()) {
      value = res->getString("v");
      return true;
    }
    return false;
  } catch (sql::SQLException &e) {
    cerr << "SQL error (read): " << e.what() << endl;
    return false;
  }
}

// remove a key-value pair from database
bool DBHandler::remove(const string& key) {
  try {
    unique_ptr<sql::PreparedStatement> pstmt {
      con->prepareStatement(
        "DELETE FROM kv_pairs WHERE k = ?"
      )
    };
    pstmt->setString(1, key);
    int affected = pstmt->executeUpdate();
    return affected > 0;
  } catch (sql::SQLException &e) {
    cerr << "SQL error (delete): " << e.what() << endl;
    return false;
  }
}