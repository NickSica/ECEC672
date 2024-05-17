#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <chrono>
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

  //// Need to do smallest means first
  // std::sort(in_delays.begin(), in_delays.end(), [](const vector<float> &a,
  // const vector<float> &b){
  //   return a[0] < b[0];
  // });

  auto max_iter = in_delays.begin();
  vector<float> new_max = *max_iter;
  vector<float> crit_max = *max_iter;
  int max_idx = 0;
  int idx = 0;
  for (auto i = std::next(in_delays.begin()); i != in_delays.end(); i++) {
    new_max = maximum(new_max, *i);
    float sigmaA = standard_deviation(crit_max);
    float sigmaB = standard_deviation(*i);
    float rho = correlation_coefficient(crit_max, *i, sigmaA, sigmaB);
    float theta = combined_standard_deviation(sigmaA, sigmaB, rho);
    float x = normalize_difference(crit_max, *i, theta);
    float phi = cumulative_distribution_function(x);
    if (phi < 0.5) {
      max_idx = idx;
    }
    idx++;
  }
  float gate_cost = in_cost[max_idx];
  string crit_path = crit_paths[max_idx] + "->" + output;
  vector<float> new_delay = add(new_max, curr_gate->strength->delays);
  return make_tuple(std::move(new_delay), gate_cost, crit_path);
}

tuple<string, string, float, float> stat_sta() {
  float delay;
  float cost;
  float max_delay = 0;
  float max_cost;
  string crit_path_vars;
  string crit_path;
  string max_cpv;
  string max_crit_path;
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
    if (delay >= max_delay) {
      max_delay = delay;
      max_cpv = crit_path;
      max_crit_path = crit_path;
      max_cost = cost;
    }

    cout << "Path delay for " << output << " is " << delay << endl;
    cout << "Path cost for " << output << " is " << cost << endl;
  }
  return make_tuple(max_crit_path, max_cpv, max_delay, max_cost);
}

tuple<string, string, float, float> optimize() {
  auto [crit_path, crit_path_vars, delay, cost] = stat_sta();
  float cost_fit = delay * cost;
  for (auto &pair : gates) {
    for (auto &gate : pair.second) {
      string gate_strength = gate.strength->gate;
      int drive_idx = gate_strength.find_last_not_of("1234567890");
      int drive = stoi(gate_strength.substr(drive_idx + 1));
      int new_drive = drive + 1;
      float new_cost_fit = cost_fit;
      while (new_cost_fit <= cost_fit) {
        if (lib_time.find(pair.first + to_string(new_drive)) ==
            lib_time.end()) {
          break;
        } else {
          auto old_strength = gate.strength;
          gate.strength = &lib_time[pair.first + to_string(new_drive)];
          int bad_gate = 0;
          for (auto &output : gate.outputs) {
            if (output->name == "") {
              bad_gate = 1;
              break;
            }

            reverse_gates[output->name].gate->strength =
                &lib_time[pair.first + to_string(new_drive)];
          }
          if (bad_gate == 1) {
            break;
          }
          cout << "Switching " << old_strength->gate << " for "
               << gate.strength->gate << endl;
          auto [new_crit_path, new_crit_path_vars, new_delay, new_cost] =
              stat_sta();
          new_cost_fit = new_delay * new_cost;
          cout << "Delay before: " << delay << endl;
          cout << "Delay after: " << new_delay << endl;
          cout << "Cost before: " << cost << endl;
          cout << "Cost after:  " << new_cost << endl;
          cout << "Cost fit before: " << cost_fit << endl;
          cout << "Cost fit after:  " << new_cost_fit << endl << endl;

          if (new_cost_fit > cost_fit) {
            gate.strength = old_strength;
            for (auto &output : gate.outputs) {
              reverse_gates[output->name].gate->strength = old_strength;
            }
          } else {
            tie(crit_path, crit_path_vars, delay, cost) =
                tie(new_crit_path, new_crit_path_vars, new_delay, new_cost);
            gate_strength = gate.strength->gate;
            drive_idx = gate_strength.find_last_not_of("1234567890");
            drive = stoi(gate_strength.substr(drive_idx + 1));
            new_drive = drive + 1;
            delay = new_delay;
            cost_fit = new_cost_fit;
            cost = new_cost;
          }
        }
      }
    }
  }
  return tie(crit_path, crit_path_vars, delay, cost);
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
        << "Benchmark,Critical Path,Critical Path Delay,Cost($),Run Time(ms)"
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
      auto t1 = chrono::high_resolution_clock::now();
      auto [crit_path, crit_path_vars, delay, cost] = optimize();

      auto t2 = chrono::high_resolution_clock::now();
      auto run_time = chrono::duration_cast<chrono::milliseconds>(t2 - t1);

      csv_file << benchmark << "," << crit_path << "," << crit_path_vars << ","
               << cost << "," << run_time.count() << endl;
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
    auto [crit_path, crit_path_vars, delay, cost] = optimize();
  }

  return 0;
}
