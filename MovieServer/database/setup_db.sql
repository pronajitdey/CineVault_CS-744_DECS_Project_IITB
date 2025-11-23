CREATE DATABASE IF NOT EXISTS movie_store;
CREATE USER IF NOT EXISTS 'movieuser'@'localhost' IDENTIFIED BY 'moviepass';
GRANT ALL PRIVILEGES ON movie_store.* TO 'movieuser'@'localhost';
FLUSH PRIVILEGES;

USE movie_store;
CREATE TABLE IF NOT EXISTS movies (
  id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  title VARCHAR(255) NOT NULL,
  genre VARCHAR(100),
  release_year INT,
  rating DECIMAL(2,1),
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

SELECT 'movie_store database and movies table created successfully' AS status;