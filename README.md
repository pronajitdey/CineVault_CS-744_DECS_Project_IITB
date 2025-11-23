# 🎬CineVault - A Movie Store System

CineVault is a fast, lightweight and reliable **HTTP-based Movie Store System** built in **C++**, using:

- [`cpp-httplib`](https://github.com/yhirose/cpp-httplib) for the HTTP server.
- [`jsoncons`](https://github.com/danielaparker/jsoncons) for JSON parsing and formatting
- **MySQL Connector/C++ (JDBC API)** for persistent database storage.
- A custom **LRU Cache** to reduce database load during read-heavy workloads.

This project is developed as part of the **CS744 – Design and Engineering of Computing Systems** course at **IIT Bombay**.

## Features

- Add, list, search, update and delete movies
- Persistent MySQL storage
- LRU-based in-memory cache for faster reads
- JSON-formatted responses for easy frontend integration
- Thread-safe request handling with `std::mutex` and `std::lock_guard`
- Modular structure: `main.cpp`, `db.cpp`, `cache.cpp`, and headers `db.h` and `cache.h`

## Database Setup

1. Install MySQL database: `sudo apt install mysql-server`
2. Move to project directory `Movie_store_system`
3. Load from file:

```
sudo mysql < ./MovieServer/database/setup_db.sql
```

4. Database can be viewed by:

```
mysql -u movieuser -p
Enter password: moviepass

mysql> USE movie_store;
mysql> SELECT * FROM movies;
```

## Build instructions

1. Install dependencies:

```
sudo apt install g++ cmake libmysqlcppconn-dev
```

2. Build:

```
cd ./MovieServer
mkdir build && cd build
cmake ..
make -j$(nproc)
```

3. Run Server: `./MovieHTTPServer`
   <br>
   Server starts at `http://0.0.0.0:8080`

## API Endpoints

| Method | Endpoint         | Description           |
| :----- | :--------------- | :-------------------- |
| POST   | `/add-movie`     | Add a new movie       |
| GET    | `/list-movies`   | List all movies       |
| GET    | `/search-movie`  | Search movie by title |
| PUT    | `/update-rating` | Update rating         |
| DELETE | `/delete-movie`  | Remove a movie        |

## Example `curl` Commands

**Add a movie**

```
curl -X POST --data "title=Kishmish&genre=Romance, Comedy&release-year=2022&rating=6.6" http://localhost:8080/add-movie

or

curl -X POST http://localhost:8080/add-movie -H "Content-Type: application/x-www-form-urlencoded" -d "title=Avengers: Infinity War" -d "genre=Action, Science fiction" -d "release-year=2018" -d "rating=8.4"
```

**List movies**

```
curl -X GET http://localhost:8080/list-movies
```

**Search movie by title**

```
curl -X GET "http://localhost:8080/search-movie?title=inception"

or (for multi-word title)

curl -X GET "http://localhost:8080/search-movie" -G --data-urlencode "title=777 charlie"
```

**Update rating**

```
curl -X PUT http://localhost:8080/update-rating -H "Content-Type: application/x-www-form-urlencoded" -d "id=10&rating=9.9"
```

**Delete movie**

```
curl -X DELETE "http://localhost:8080/delete-movie?id=10"
```

## Cache Behavior

- Listing movies caches all movie details with key `list_movies`
- When a movie is added, it is cached with key `movie: <title>` and `list_movies` is evicted.
- Updating a movie rating makes it the most recent in cache and `list_movies` is evicted.
- Deleting a movie evicts it's key and `list_movies` from cache.

The caches uses **LRU (Least Recently Used)** replacement policy implemented with:

- A `std::list` to maintain order of access (LRU order), front of list is most recent and back of list is least recent
- A `std::unordered_map` for O(1) lookups
- `std::mutex` and `std::lock_guard` for thread-safety

## Performance & Scaling

**Current Performance:**

Read-Heavy Workload: CPU bottleneck

Write-Heavy Workload: I/O bottleneck

Connection Management
Uses per-thread connection pooling: each of default 8 threads maintains one persistent MySQL connection for reuse.

Enforces connection limit of 100 (safety mechanism that clears all connections when limit reached, forcing threads to create new ones).

Bottleneck Analysis
Reads limited by: CPU (95% utilization), not I/O or memory

Writes limited by: Disk I/O (95% utilization, I/O await time of 675 ms)

Scaling Strategies
To scale reads: Increase CPU cores, add database read replicas, or optimize JSON serialization

To scale writes: Use NVMe storage,or implement write batching
