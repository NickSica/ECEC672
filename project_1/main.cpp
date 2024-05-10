#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <vector>

namespace fs = std::filesystem;
int DEBUG = 0;

using namespace boost::algorithm;
using namespace std;

typedef struct Timing {
  float cost;
  vector<float> delays;
} Timing;

struct _Gate;
typedef struct _Gate Gate;

typedef struct Wire {
  vector<Gate *> gates;
  map<string, vector<float>> delay_vars;
} Wire;

struct _Gate {
  string type;
  Timing *strength;
  vector<Wire *> inputs;
  vector<Wire *> outputs;

  _Gate(string type, Timing *strength, vector<Wire *> inputs,
        vector<Wire *> outputs)
      : type(type), strength(strength), inputs(inputs), outputs(outputs) {}
};

typedef struct Reverse_Gate {
  // Gate *gate;
  shared_ptr<Gate> gate;
  vector<string> inputs;
} Reverse_Gate;

// typedef struct Gates {
//   vector<Gate> nands;
//   vector<Gate> ands;
//   vector<Gate> nors;
//   vector<Gate> ors;
//   vector<Gate> xors;
//   vector<Gate> xnors;
//   vector<Gate> buffs;
//   vector<Gate> nots;
// } Gates;

map<string, Timing> lib_time;
map<string, Wire> wires;
vector<string> inputs;
vector<string> outputs;
map<string, vector<Gate>> gates;
map<string, Reverse_Gate> reverse_gates;

int read_timings() {
  std::string line;
  ifstream file;
  file.open("cell_library.time");
  if (!file) {
    cerr << "Error opening cell library timing" << endl;
    file.close();
    return -1;
  }
  while (getline(file, line)) {
    trim(line);
    if (line[0] == '#' || line.empty()) {
      continue;
    }

    std::string gate = line.substr(line.find_first_of(' ') + 1);

    std::string op_line;
    getline(file, op_line);
    trim(op_line);
    std::string op = op_line.substr(op_line.find_first_of(' ') + 1);

    std::string cost_line;
    getline(file, cost_line);
    trim(cost_line);
    float cost = stof(cost_line.substr(cost_line.find_first_of(' ') + 1));
    if (DEBUG == 1) {
      cout << op << endl;
      cout << cost_line.substr(cost_line.find_first_of(' ') + 1) << endl;
    }

    std::string delay_line;
    getline(file, delay_line);
    trim(delay_line);
    string raw_delays = delay_line.substr(delay_line.find_first_of(' ') + 1);
    vector<string> str_delays;
    split(str_delays, raw_delays, boost::is_any_of(" "));
    Timing t = {cost, {}};
    int i = 0;
    for (auto &v : str_delays) {
      if (DEBUG == 1) {
        cout << v << endl;
      }
      t.delays.push_back(stof(v));
      i++;
    }

    lib_time[gate] = t;
  }
  file.close();
  return 0;
}

int read_benchmark(string folder_name, string benchmark) {
  std::string line;
  ifstream file{folder_name + "/" + benchmark + ".time"};
  if (!file) {
    cerr << "Error opening benchmark timing" << endl;
    file.close();
    return -1;
  }

  cout << "Parsing benchmark timing" << endl;
  while (getline(file, line)) {
    trim(line);
    if (line[0] == '#' || line.empty()) {
      continue;
    }

    size_t idx;
    size_t idx2;
    idx = line.find_first_of("\t ");
    string start_wire = line.substr(0, idx);
    if (DEBUG == 1) {
      cout << line << endl;
      cout << start_wire << endl;
    }

    string stop_wire;
    string var;
    for (int i = 0; i < 5; i++) {
      idx2 = line.find_first_not_of("\t ", idx);
      idx = line.find_first_of("\t ", idx2);
      var = line.substr(idx2, idx - idx2);
      if (i == 0) {
        stop_wire = var;
        wires[start_wire].delay_vars[var].reserve(4);
      } else {
        wires[start_wire].delay_vars[stop_wire].push_back(stof(var));
      }

      if (DEBUG == 1) {
        cout << var << endl;
      }
    }
    if (DEBUG == 1) {
      cout << endl;
    }
  }
  if (DEBUG == 1)
    cout << endl << endl;
  file.close();
  file.clear();

  file.open(folder_name + "/" + benchmark + ".bench");
  if (!file) {
    cerr << "Error opening benchmark circuit" << endl;
    file.close();
    return -1;
  }

  cout << "Parsing benchmark file" << endl;
  while (getline(file, line)) {
    trim(line);
    if (line[0] == '#' || line.empty()) {
      continue;
    }

    size_t idx;
    if (DEBUG == 1)
      cout << line << endl;
    if ((idx = line.find("INPUT(")) != string::npos) {
      idx += 6;
      if (DEBUG == 1)
        cout << "New input: " << line.substr(idx, line.length() - idx - 1)
             << endl;
      inputs.push_back(line.substr(idx, line.length() - idx - 1));
    } else if ((idx = line.find(" = ")) != string::npos) {
      string out = line.substr(0, idx);

      idx += 3;
      int parse_idx = line.find("(");
      string gate = line.substr(idx, parse_idx - idx);
      parse_idx += 1;

      string raw_inputs = line.substr(parse_idx, line.length() - parse_idx - 1);

      if (DEBUG == 1) {
        cout << "Gate: " << gate;
        cout << " Outputs: " << out;
      }

      vector<string> inputs;
      split_regex(inputs, raw_inputs, boost::regex(", "));

      vector<Wire *> gate_in;
      vector<Wire *> gate_out;
      for (auto input : inputs) {
        gate_in.push_back(&wires[input]);

        if (DEBUG == 1) {
          cout << " Input: " << input;
        }
      }

      gate_out.push_back(&wires[out]);
      // Gate new_gate = {gate, &lib_time[gate + "1"], gate_in, gate_out};
      // gates[gate].push_back(new_gate);
      Gate &new_gate = gates[gate].emplace_back(gate, &lib_time[gate + "1"],
                                                gate_in, gate_out);

      for (auto input : inputs) {
        wires[input].gates.push_back(&new_gate);
      }

      Reverse_Gate r_gate = {make_shared<Gate>(new_gate), inputs};
      reverse_gates[out] = r_gate;
    } else if ((idx = line.find("OUTPUT(")) != string::npos) {
      idx += 7;
      if (DEBUG == 1) {
        cout << "New output: " << line.substr(idx, line.length() - idx - 1)
             << endl;
      }
      outputs.push_back(line.substr(idx, line.length() - idx - 1));
    }
    if (DEBUG == 1)
      cout << endl;
  }
  file.close();

  return 0;
}

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
      add_delay(&in_delay, &get<0>(crit_tuple), &new_in_delay);
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

  auto max_iter = in_delays.begin();
  vector<float> new_max = *max_iter;
  int max_idx = 0;
  int idx = 0;
  for (auto i = max_iter++; i != in_delays.end(); i++) {
    int which_max = max_delay(&new_max, &*i);
    if (which_max == 2) {
      max_idx = idx;
    }
    idx++;
  }
  float gate_cost = in_cost[max_idx];
  string crit_path = crit_paths[max_idx] + "->" + output;
  vector<float> new_delay;
  add_delay(&new_max, &curr_gate->strength->delays, &new_delay);
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
