#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <vector>

#include "louds-patricia.hpp"

using namespace std;
using namespace std::chrono;

int main() {
  ios_base::sync_with_stdio(false);
  vector<string> keys;
  string line;
  while (getline(cin, line)) {
    keys.push_back(line);
  }

  high_resolution_clock::time_point begin = high_resolution_clock::now();
  louds::Patricia patricia;
  for (uint64_t i = 0; i < keys.size(); ++i) {
    patricia.add(keys[i]);
  }
  patricia.build();
  high_resolution_clock::time_point end = high_resolution_clock::now();
  double elapsed = (double)duration_cast<nanoseconds>(end - begin).count();
  cout << "build = " << (elapsed / keys.size()) << " ns/key" << endl;

  cout << "#keys = " << patricia.n_keys() << endl;
  cout << "#nodes = " << patricia.n_nodes() << endl;
  cout << "size = " << patricia.size() << " bytes" << endl;

  vector<int64_t> ids(keys.size());

  begin = high_resolution_clock::now();
  for (uint64_t i = 0; i < keys.size(); ++i) {
    ids[i] = patricia.lookup(keys[i]);
    assert(ids[i] != -1);
  }
  end = high_resolution_clock::now();
  elapsed = (double)duration_cast<nanoseconds>(end - begin).count();
  cout << "seq. lookup = " << (elapsed / keys.size()) << " ns/key" << endl;

  sort(ids.begin(), ids.end());
  for (uint64_t i = 0; i < ids.size(); ++i) {
    if (i != (uint64_t)ids[i]) {
      cout << "i = " << i << ", ids[i] = " << ids[i] << endl;
    }
    assert((uint64_t)ids[i] == i);
  }

  random_shuffle(keys.begin(), keys.end());

  begin = high_resolution_clock::now();
  for (uint64_t i = 0; i < keys.size(); ++i) {
    assert(patricia.lookup(keys[i]) != -1);
  }
  end = high_resolution_clock::now();
  elapsed = (double)duration_cast<nanoseconds>(end - begin).count();
  cout << "rnd. lookup = " << (elapsed / keys.size()) << " ns/key" << endl;

  return 0;
}