#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <vector>

int DEBUG = 1;

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
  Gate *gate;
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
    cout << "Error opening cell library timing" << endl;
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
    cout << line << endl;
    if ((idx = line.find("INPUT(")) != string::npos) {
      idx += 6;
      cout << "New input: " << line.substr(idx, line.length() - idx - 1)
           << endl;
      inputs.push_back(line.substr(idx, line.length() - idx - 1));
    } else if ((idx = line.find(" = ")) != string::npos) {
      string out = line.substr(0, idx);

      idx += 3;
      int pars_idx = line.find("(");
      string gate = line.substr(idx, pars_idx - idx);
      pars_idx += 1;
      idx = line.find(", ");
      string in1 = line.substr(pars_idx, idx - pars_idx);
      idx += 2;
      string in2 = line.substr(idx, line.length() - idx - 1);
      if (DEBUG == 1) {
        cout << "Gate: " << gate;
        cout << " Outputs: " << out;
        cout << " Input1: " << in1;
        cout << " Input2: " << in2 << endl;
      }

      vector<Wire *> gate_in;
      vector<Wire *> gate_out;
      gate_in.push_back(&wires[in1]);
      gate_in.push_back(&wires[in2]);
      gate_out.push_back(&wires[out]);
      // Gate new_gate = {gate, &lib_time[gate + "1"], gate_in, gate_out};
      // gates[gate].push_back(new_gate);
      Gate &new_gate = gates[gate].emplace_back(gate, &lib_time[gate + "1"],
                                                gate_in, gate_out);
      wires[in1].gates.push_back(&new_gate);
      wires[in2].gates.push_back(&new_gate);

      vector<string> ins = {in1, in2};
      Reverse_Gate r_gate = {&new_gate, ins};
      reverse_gates[out] = r_gate;
    } else if ((idx = line.find("OUTPUT(")) != string::npos) {
      idx += 7;
      if (DEBUG == 1) {
        cout << "New output: " << line.substr(idx, line.length() - idx - 1)
             << endl;
      }
      outputs.push_back(line.substr(idx, line.length() - idx - 1));
    }
    cout << endl;
  }
  file.close();

  return 0;
}

int max_delay(vector<float> *delay1, vector<float> *delay2,
              vector<float> *max_delay) {
  // FIXME this isn't right
  // If in1 is the max return 1, otherwise return 2
  if (delay1[0] > delay2[0]) {
    max_delay->push_back(delay1->at(0));
    return 1;
  }

  max_delay->push_back(delay2->at(0));
  return 2;
}

void add_delay(vector<float> *delay1, vector<float> *delay2,
               vector<float> *delay_out) {
  // FIXME this isn't right
  delay_out->push_back(delay1->at(0) + delay2->at(0));
}

float calc_delay(vector<float> delay_vars) {
  // FIXME This isn't right, just need to get rid of errors for unused variables
  return delay_vars[0];
}

tuple<vector<float>, float> calc_crit_path(string output) {
  string in1 = reverse_gates[output].inputs[0];
  string in2 = reverse_gates[output].inputs[1];

  Gate *curr_gate = reverse_gates[output].gate;
  int in1_is_input = (find(inputs.begin(), inputs.end(), in1) != inputs.end());
  int in2_is_input = (find(inputs.begin(), inputs.end(), in2) != inputs.end());
  vector<float> in1_delay = wires[in1].delay_vars[output];
  vector<float> in2_delay = wires[in2].delay_vars[output];
  vector<float> new_in1_delay;
  vector<float> new_in2_delay;
  float in1_cost = curr_gate->strength->cost;
  float in2_cost = curr_gate->strength->cost;
  if (DEBUG == 1) {
    cout << "At output: " << output << endl;
    cout << "Inputs are: " << in1 << " and " << in2 << endl;
  }
  if (!in1_is_input) {
    tuple<vector<float>, float> crit_tuple = calc_crit_path(in1);
    // TODO: Might be able to just change in1_delay and get rid of the last var
    add_delay(&in1_delay, &get<0>(crit_tuple), &new_in1_delay);
    in1_cost = curr_gate->strength->cost + get<1>(crit_tuple);
  } else {
    new_in1_delay = in1_delay;
  }

  if (!in2_is_input) {
    tuple<vector<float>, float> crit_tuple = calc_crit_path(in2);
    add_delay(&in2_delay, &get<0>(crit_tuple), &new_in2_delay);
    in2_cost = curr_gate->strength->cost + get<1>(crit_tuple);
  } else {
    new_in2_delay = in2_delay;
  }

  vector<float> new_max;
  int which_max = max_delay(&new_in1_delay, &new_in2_delay, &new_max);
  float gate_cost = in1_cost;
  if (which_max == 2) {
    gate_cost = in2_cost;
  }
  vector<float> new_delay;
  add_delay(&new_max, &curr_gate->strength->delays, &new_delay);
  return make_tuple(std::move(new_delay), gate_cost);
}

void get_critical_path() {
  for (string output : outputs) {
    string curr_wire = output;
    tuple<vector<float>, float> path_tuple = calc_crit_path(output);
    float delay = calc_delay(get<0>(path_tuple));
    cout << "Path delay for " << output << " is " << delay << endl;
    cout << "Path cost for " << output << " is " << get<1>(path_tuple) << endl;
  }
}

int main() {
  read_timings();
  read_benchmark("BENCHMARKS/Combinational1PO", "c17_1PO");
  get_critical_path();

  return 0;
}
