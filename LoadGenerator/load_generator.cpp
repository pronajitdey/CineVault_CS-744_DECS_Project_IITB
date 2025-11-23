#include <iostream>
#include <vector>
#include <thread>
#include <chrono>
#include <random>
#include <atomic>
#include <mutex>
#include <httplib.h>
#include <fstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

using namespace std;
using namespace chrono;

// Configuration
struct Config {
  string server_host = "localhost";
  int server_port = 8080;
  int num_threads = 4;
  int duration_seconds = 60;
  int warmup_seconds = 10;
  string workload_type = "mixed"; // read, write, mixed, search, update
  double read_ratio = 0.7;  // For mixed workload
  double write_ratio = 0.2;
  double search_ratio = 0.1;
  int think_time_ms = 0;  // Think time between requests
};

// Statistics
struct Stats {
  atomic<long long> total_requests{0};
  atomic<long long> successful_requests{0}; //also includes not_found_requests
  atomic<long long> failed_requests{0};
  atomic<long long> not_found_requests{0};

  atomic<long long> add_count{0};
  atomic<long long> list_count{0};
  atomic<long long> search_count{0};
  atomic<long long> update_count{0};
  atomic<long long> delete_count{0};

  vector<double> latencies;
  mutex latency_mutex;

  void record_latency(double latency_ms) {
    lock_guard<mutex> lock(latency_mutex);
    latencies.push_back(latency_ms);
  }

  void print_stats(int duration) {
    cout << "\n========== LOAD TEST RESULTS ==========\n";
    cout << "Duration: " << duration << " seconds\n";
    cout << "Total Requests: " << total_requests << "\n";
    cout << "Successful (200): " << (successful_requests - not_found_requests) << "\n";
    cout << "Not Found (500): " << not_found_requests << "\n";
    cout << "Failed: " << failed_requests << "\n";
    
    // Success rates
    double actual_success_rate = 0.0;
    double server_response_rate = 0.0;
    
    if (total_requests > 0) {
      actual_success_rate = (double)(successful_requests - not_found_requests) / total_requests * 100;
      server_response_rate = (double)successful_requests / total_requests * 100;
    }
    
    cout << "\nSuccess Rates:\n";
    cout << "  Actual Operations: " << fixed << setprecision(2) << actual_success_rate << "%\n";
    cout << "  Server Responses: " << server_response_rate << "%\n";
    
    // Throughput
    cout << "\nThroughput:\n";
    cout << "  Total: " << fixed << setprecision(2) 
        << (double)total_requests / duration << " req/s\n";
    cout << "  Effective: " << fixed << setprecision(2) 
        << (double)(successful_requests - not_found_requests) / duration << " req/s\n";

    cout << "\nRequest Distribution:\n";
    cout << "  ADD: " << add_count << "\n";
    cout << "  LIST: " << list_count << "\n";
    cout << "  SEARCH: " << search_count << "\n";
    cout << "  UPDATE: " << update_count << "\n";
    cout << "  DELETE: " << delete_count << "\n";

    if (!latencies.empty()) {
      sort(latencies.begin(), latencies.end());
      double sum = 0;
      for (double lat : latencies) sum += lat;

      cout << "\nLatency Statistics (ms):\n";
      cout << "  Mean: " << fixed << setprecision(2) << sum / latencies.size() << "\n";
      cout << "  Median: " << latencies[latencies.size() / 2] << "\n";
      cout << "  P95: " << latencies[(int)(latencies.size() * 0.95)] << "\n";
      cout << "  P99: " << latencies[(int)(latencies.size() * 0.99)] << "\n";
      cout << "  Min: " << latencies[0] << "\n";
      cout << "  Max: " << latencies[latencies.size() - 1] << "\n";
    }
    cout << "=======================================\n";
  }

  void export_to_csv(const string& filename) {
    ofstream file(filename);
    file << "request_id,latency_ms\n";
    lock_guard<mutex> lock(latency_mutex);
    for (size_t i = 0; i < latencies.size(); i++) {
      file << i << "," << latencies[i] << "\n";
    }
    file.close();
    cout << "Latency data exported to " << filename << "\n";
  }
};

// Movie data generator
class MovieGenerator {
private:
  vector<string> titles = {
    "The Shawshank Redemption", "The Godfather", "The Dark Knight",
    "Pulp Fiction", "Forrest Gump", "Inception", "Fight Club",
    "The Matrix", "Interstellar", "Gladiator", "The Prestige",
    "The Departed", "Whiplash", "The Lion King", "Back to the Future",
    "Spirited Away", "Parasite", "Green Book", "Joker", "1917",
    "Avengers Endgame", "Spider-Man", "Iron Man", "Batman Begins",
    "Titanic", "Avatar", "Jurassic Park", "Star Wars", "E.T.",
    "The Lord of the Rings", "Harry Potter", "The Hobbit"
  };

  vector<string> genres = {
    "Action", "Comedy", "Drama", "Thriller", "Sci-Fi",
    "Horror", "Romance", "Adventure", "Mystery", "Fantasy",
    "Action, Thriller", "Sci-Fi, Adventure", "Drama, Romance"
  };

  random_device rd;
  mt19937 gen;
  uniform_int_distribution<> title_dist;
  uniform_int_distribution<> genre_dist;
  uniform_int_distribution<> year_dist;
  uniform_real_distribution<> rating_dist;
  uniform_int_distribution<> suffix_dist;

public:
  MovieGenerator() : gen(rd()), 
                    title_dist(0, titles.size() - 1),
                    genre_dist(0, genres.size() - 1),
                    year_dist(1980, 2024),
                    rating_dist(1.0, 10.0),
                    suffix_dist(1000, 9999) {}

  string generate_title() {
    return titles[title_dist(gen)] + " " + to_string(suffix_dist(gen));
  }

  string generate_genre() {
    return genres[genre_dist(gen)];
  }

  int generate_year() {
    return year_dist(gen);
  }

  double generate_rating() {
    return rating_dist(gen);
  }

  string get_random_existing_title() {
    return titles[title_dist(gen)];
  }
};

// Load Generator Worker
class LoadWorker {
private:
  Config config;
  Stats& stats;
  MovieGenerator movie_gen;
  httplib::Client client;
  random_device rd;
  mt19937 gen;
  uniform_real_distribution<> op_dist;
  uniform_int_distribution<> id_dist;

  atomic<bool>& running;
  int worker_id;

public:
  LoadWorker(const Config& cfg, Stats& st, atomic<bool>& run, int id) 
    : config(cfg), stats(st), 
      client(cfg.server_host, cfg.server_port),
      gen(rd()), op_dist(0.0, 1.0), id_dist(1, 100),
      running(run), worker_id(id) {
    client.set_connection_timeout(5, 0);  // 5 seconds
    client.set_read_timeout(10, 0);       // 10 seconds
  }

  void run() {
    while (running) {
      double op_choice = op_dist(gen);
      auto start = steady_clock::now();
      bool success = false;

      if (config.workload_type == "read") {
        success = perform_read_operation(op_choice);
      } else if (config.workload_type == "write") {
        success = perform_write_operation(op_choice);
      } else if (config.workload_type == "search") {
        success = perform_search();
      } else if (config.workload_type == "update") {
        success = perform_update();
      } else {  // mixed
        success = perform_mixed_operation(op_choice);
      }

      auto end = steady_clock::now();
      double latency_ms = duration_cast<microseconds>(end - start).count() / 1000.0;

      stats.total_requests++;
      if (success) {
        stats.successful_requests++;
        if (latency_ms >= 0) {  // Only record valid latencies
          stats.record_latency(latency_ms);
        }
      } else {
        stats.failed_requests++;
      }

      if (config.think_time_ms > 0) {
        this_thread::sleep_for(milliseconds(config.think_time_ms));
      }
    }
  }

private:
  bool perform_read_operation(double choice) {
    if (choice < 0.7) {
      return perform_list();
    } else {
      return perform_search();
    }
  }

  bool perform_write_operation(double choice) {
    if (choice < 0.8) {
      return perform_add();
    } else if (choice < 0.95) {
      return perform_update();
    } else {
      return perform_delete();
    }
  }

  bool perform_mixed_operation(double choice) {
    if (choice < config.read_ratio) {
      return perform_list();
    } else if (choice < config.read_ratio + config.write_ratio) {
      return perform_add();
    } else {
      return perform_search();
    }
  }

  bool perform_add() {
    stats.add_count++;
    string title = movie_gen.generate_title();
    string genre = movie_gen.generate_genre();
    int year = movie_gen.generate_year();
    double rating = movie_gen.generate_rating();

    string body = "title=" + title + 
                  "&genre=" + genre + 
                  "&release-year=" + to_string(year) + 
                  "&rating=" + to_string(rating);

    auto res = client.Post("/add-movie", body, "application/x-www-form-urlencoded");
    return res && res->status == 200;
  }

  bool perform_list() {
    stats.list_count++;
    auto res = client.Get("/list-movies");
    return res && res->status == 200;
  }

  bool perform_search() {
    stats.search_count++;
    string title = movie_gen.get_random_existing_title();
    string path = "/search-movie?title=" + title;
    auto res = client.Get(path);
    return res && res->status == 200;
  }

  bool perform_update() {
    stats.update_count++;
    int id = id_dist(gen);
    double rating = movie_gen.generate_rating();

    string body = "id=" + to_string(id) + "&rating=" + to_string(rating);
    auto res = client.Put("/update-rating", body, "application/x-www-form-urlencoded");
    if (!res) return false;  // Connection failure

    if (res->status == 200) {
      return true;  // Actual success
    } else if (res->status == 500) {
      stats.not_found_requests++; 
      return true;  // Server handled it, but ID not found
    }
    return false;
  }

  bool perform_delete() {
    stats.delete_count++;
    int id = id_dist(gen);
    string path = "/delete-movie?id=" + to_string(id);
    auto res = client.Delete(path);
    if (!res) return false;  // Connection failure

    if (res->status == 200) {
      return true;  // Actual success
    } else if (res->status == 500) {
      stats.not_found_requests++;
      return true;  // Server handled it
    }
    
    return false; 
  }
};

void print_usage() {
  cout << "Usage: ./LoadGenerator [options]\n";
  cout << "Options:\n";
  cout << "  --host <hostname>        Server hostname (default: localhost)\n";
  cout << "  --port <port>            Server port (default: 8080)\n";
  cout << "  --threads <num>          Number of threads (default: 4)\n";
  cout << "  --duration <seconds>     Test duration (default: 60)\n";
  cout << "  --warmup <seconds>       Warmup period (default: 10)\n";
  cout << "  --workload <type>        Workload type: read, write, mixed, search, update (default: mixed)\n";
  cout << "  --read-ratio <ratio>     Read ratio for mixed workload (default: 0.7)\n";
  cout << "  --write-ratio <ratio>    Write ratio for mixed workload (default: 0.2)\n";
  cout << "  --think-time <ms>        Think time between requests (default: 0)\n";
  cout << "  --output <filename>      CSV output file for latencies (default: latencies.csv)\n";
  cout << "  --help                   Show this help message\n";
}

int main(int argc, char* argv[]) {
  Config config;
  string output_file = "latencies.csv";

  // Parse command line arguments
  for (int i = 1; i < argc; i++) {
    string arg = argv[i];
    if (arg == "--help") {
      print_usage();
      return 0;
    } else if (arg == "--host" && i + 1 < argc) {
      config.server_host = argv[++i];
    } else if (arg == "--port" && i + 1 < argc) {
      config.server_port = stoi(argv[++i]);
    } else if (arg == "--threads" && i + 1 < argc) {
      config.num_threads = stoi(argv[++i]);
    } else if (arg == "--duration" && i + 1 < argc) {
      config.duration_seconds = stoi(argv[++i]);
    } else if (arg == "--warmup" && i + 1 < argc) {
      config.warmup_seconds = stoi(argv[++i]);
    } else if (arg == "--workload" && i + 1 < argc) {
      config.workload_type = argv[++i];
    } else if (arg == "--read-ratio" && i + 1 < argc) {
      config.read_ratio = stod(argv[++i]);
    } else if (arg == "--write-ratio" && i + 1 < argc) {
      config.write_ratio = stod(argv[++i]);
    } else if (arg == "--think-time" && i + 1 < argc) {
      config.think_time_ms = stoi(argv[++i]);
    } else if (arg == "--output" && i + 1 < argc) {
      output_file = argv[++i];
    }
  }

  cout << "========== LOAD GENERATOR ==========\n";
  cout << "Server: " << config.server_host << ":" << config.server_port << "\n";
  cout << "Threads: " << config.num_threads << "\n";
  cout << "Workload: " << config.workload_type << "\n";
  cout << "Duration: " << config.duration_seconds << "s (+" 
       << config.warmup_seconds << "s warmup)\n";
  if (config.workload_type == "mixed") {
    cout << "Read Ratio: " << config.read_ratio << "\n";
    cout << "Write Ratio: " << config.write_ratio << "\n";
  }
  cout << "====================================\n\n";

  // Test server connectivity
  httplib::Client test_client(config.server_host, config.server_port);
  auto test_res = test_client.Get("/hi");
  if (!test_res || test_res->status != 200) {
    cerr << "Error: Cannot connect to server at " 
         << config.server_host << ":" << config.server_port << "\n";
    cerr << "Make sure the server is running.\n";
    return 1;
  }
  cout << "Server connectivity: OK\n\n";

  Stats stats;
  atomic<bool> running{true};
  vector<thread> workers;

  // Warmup phase
  if (config.warmup_seconds > 0) {
    cout << "Starting warmup phase for " << config.warmup_seconds << " seconds...\n";
    Stats warmup_stats;
    atomic<bool> warmup_running{true};
    vector<thread> warmup_workers;

    for (int i = 0; i < config.num_threads; i++) {
      warmup_workers.emplace_back([&, i]() {
        LoadWorker worker(config, warmup_stats, warmup_running, i);
        worker.run();
      });
    }

    this_thread::sleep_for(seconds(config.warmup_seconds));
    warmup_running = false;

    for (auto& worker : warmup_workers) {
      worker.join();
    }
    cout << "Warmup complete. Starting actual test...\n\n";
  }

  // Start load generation
  auto test_start = steady_clock::now();

  for (int i = 0; i < config.num_threads; i++) {
    workers.emplace_back([&, i]() {
      LoadWorker worker(config, stats, running, i);
      worker.run();
    });
  }

  // Progress monitoring
  for (int i = 0; i < config.duration_seconds; i++) {
    this_thread::sleep_for(seconds(1));
    if (i % 10 == 0 && i > 0) {
      cout << "Progress: " << i << "/" << config.duration_seconds 
           << "s - Requests: " << stats.total_requests 
           << " (Success: " << stats.successful_requests 
           << ", Failed: " << stats.failed_requests << ")\n";
    }
  }

  // Stop workers
  running = false;
  for (auto& worker : workers) {
    worker.join();
  }

  auto test_end = steady_clock::now();
  int actual_duration = duration_cast<seconds>(test_end - test_start).count();

  // Print results
  stats.print_stats(actual_duration);
  stats.export_to_csv(output_file);

  return 0;
}
