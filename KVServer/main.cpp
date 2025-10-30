#include <iostream>
#include "httplib.h"

using namespace std;

int main() {
  httplib::Server svr;

  svr.Get("/hi", [](const httplib::Request &, httplib::Response &res) {
    res.set_content("Hello World!", "text/plain");
  });

  cout << "Server running at http://localhost:8080" << endl;
  svr.listen("0.0.0.0", 8080);
}