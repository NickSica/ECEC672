#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

#include "file_parse.hpp"
#include "math_functions.hpp"

namespace fs = std::filesystem;
int DEBUG{0};
std::map<std::string, Timing> lib_time;
std::map<std::string, Wire> wires;
std::vector<std::string> inputs;
std::vector<std::string> outputs;
std::map<std::string, std::vector<Gate>> gates;
std::map<std::string, Reverse_Gate> reverse_gates;

using namespace boost::algorithm;
using namespace std;

/*
int max_delay(vector<float> *delay1, vector<float> *delay2) {
  // FIXME this isn't right
  // If in1 is the max return 1, otherwise return 2
  if (delay2[0] > delay1[0]) {
    delay1->at(0) = delay2->at(0);
    return 2;
  }

  return 1;
}

void add_delay(vector<float> *delay1, vector<float> *delay2,
               vector<float> *delay_out) {
  // FIXME this isn't right
  delay_out->push_back(delay1->at(0) + delay2->at(0));
}

float calc_delay(vector<float> delay_vars) {
  // FIXME This isn't right, just need to get rid of errors for unused
  // variables
  return delay_vars[0];
}
*/

tuple<vector<float>, float, string> calc_crit_path(string output) {
  shared_ptr<Gate> curr_gate = reverse_gates[output].gate;

  vector<vector<float>> in_delays;
  vector<float> in_cost;
  vector<string> crit_paths;
  for (auto &in : reverse_gates[output].inputs) {
    int is_input = (find(inputs.begin(), inputs.end(), in) != inputs.end());
    vector<float> in_delay = wires[in].delay_vars[output];
    vector<float> new_in_delay;
    in_cost.push_back(curr_gate->strength->cost);
    if (!is_input) {
      tuple<vector<float>, float, string> crit_tuple = calc_crit_path(in);
      // TODO: Might be able to just change in1_delay and get rid of the last
      // var
      new_in_delay = add(in_delay, get<0>(crit_tuple));
      in_cost.back() += get<1>(crit_tuple);
      crit_paths.push_back(get<2>(crit_tuple));
    } else {
      crit_paths.push_back(in);
      new_in_delay = in_delay;
    }
    in_delays.push_back(new_in_delay);
  }
  if (DEBUG == 1) {
    cout << "At output: " << output << endl;
    // cout << "Inputs are: " << in1 << " and " << in2 << endl;
  }

  // Need to do smallest means first
  std::sort(in_delays.begin(), in_delays.end(), [](const vector<float> &a, const vector<float> &b){
    return a[0] < b[0];
  });

  auto max_iter = in_delays.begin();
  vector<float> new_max = *max_iter;
  int max_idx = 0;
  int idx = 0;
  for (auto i = max_iter++; i != in_delays.end(); i++) {
    int which_max = maximum(&new_max, &*i);
    if (which_max == 1) {
      max_idx = idx;
    }
    idx++;
  }
  float gate_cost = in_cost[max_idx];
  string crit_path = crit_paths[max_idx] + "->" + output;
  vector<float> new_delay = add(new_max, curr_gate->strength->delays);
  return make_tuple(std::move(new_delay), gate_cost, crit_path);
}

tuple<string, string, float, float> get_critical_path() {
  float delay;
  float cost;
  string crit_path_vars;
  string crit_path;
  for (string output : outputs) {
    string curr_wire = output;
    vector<float> delay_vars;
    tie(delay_vars, cost, crit_path) = calc_crit_path(output);
    stringstream delay_vars_ss;
    for (auto i : delay_vars) {
      delay_vars_ss << i << " ";
    }
    crit_path_vars = delay_vars_ss.str();
    delay = calc_delay(delay_vars);
    cout << "Path delay for " << output << " is " << delay << endl;
    cout << "Path cost for " << output << " is " << cost << endl;
  }
  return make_tuple(crit_path, crit_path_vars, delay, cost);
}

int main(int argc, char *argv[]) {
  if (argc == 1) {
    cerr << "Please input a file" << endl;
    return 1;
  }

  read_timings();

  string folder;
  string benchmark;
  // Run all
  if (strcmp(argv[1], "-r") == 0) {
    ofstream csv_file{"output.csv", ofstream::out | ofstream::trunc};
    csv_file
        << "Benchmark,Critical Path,Critical Path Delay,Cost($),Run Time(s)"
        << endl;
    folder = argv[2];
    for (const auto &entry : fs::directory_iterator(folder)) {
      cout << entry.path() << endl;
      auto file = fs::path(entry).filename();
      if (file.extension() == ".time" || file.extension() == "")
        continue;
      benchmark = file.stem();
      cout << "--------------------------------------" << endl;
      cout << "Running benchmark " << benchmark << endl;
      cout << "--------------------------------------" << endl;
      read_benchmark(folder, benchmark);
      auto [crit_path, crit_path_vars, delay, cost] = get_critical_path();
      // FIXME
      int run_time = 4;

      csv_file << benchmark << ","
               << crit_path
               << "," << crit_path_vars << "," << delay << "," << cost << "," << run_time << endl;
      wires.clear();
      inputs.clear();
      outputs.clear();
      gates.clear();
      reverse_gates.clear();
    }
    csv_file.close();
  } else {
    folder = argv[1];
    benchmark = argv[2];
    read_benchmark(folder, benchmark);
    get_critical_path();
  }

  return 0;
}
