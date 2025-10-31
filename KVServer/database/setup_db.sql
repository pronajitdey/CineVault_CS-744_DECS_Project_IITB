CREATE DATABASE IF NOT EXISTS kvstore;
CREATE USER IF NOT EXISTS 'kvdbuser'@'localhost' IDENTIFIED BY 'kvdbpass';
GRANT ALL PRIVILEGES ON kvstore.* TO 'kvdbuser'@'localhost';
FLUSH PRIVILEGES;

USE kvstore;
CREATE TABLE IF NOT EXISTS kv_pairs (
  id INT UNSIGNED AUTO_INCREMENT PRIMARY KEY,
  k VARCHAR(255) NOT NULL UNIQUE,
  v TEXT NOT NULL
);

SELECT 'kvstore database and kv_pairs table created successfully' AS status;