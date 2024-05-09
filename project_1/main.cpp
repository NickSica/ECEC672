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
  float delays[4];
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
      t.delays[i] = stof(v);
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

float max_delay(float delay1, float delay2) {
  // FIXME this isn't right should use arrays
  return max(delay1, delay2);
}

float add_delay(float delay1, float delay2) {
  // FIXME this isn't right should use arrays
  return delay1 + delay2;
}

float calc_delay(vector<float> delay_vars) {
  // FIXME This isn't right, just need to get rid of errors for unused variables
  return delay_vars[0];
}

float calc_gate_delay(float delay_vars[4]) {
  // FIXME This isn't right, just need to get rid of errors for unused variables
  return delay_vars[0];
}

float find_path(string output) {
  string in1 = reverse_gates[output].inputs[0];
  string in2 = reverse_gates[output].inputs[1];

  Gate *curr_gate = reverse_gates[output].gate;
  int in1_is_input = (find(inputs.begin(), inputs.end(), in1) != inputs.end());
  int in2_is_input = (find(inputs.begin(), inputs.end(), in2) != inputs.end());
  float gate_delay = calc_gate_delay(curr_gate->strength->delays);
  float in1_delay = calc_delay(wires[in1].delay_vars[output]);
  float in2_delay = calc_delay(wires[in2].delay_vars[output]);
  if (DEBUG == 1) {
    cout << "At output: " << output << endl;
    cout << "Inputs are: " << in1 << " and " << in2 << endl;
    cout << "Gate delay is: " << gate_delay << endl;
    cout << "Input delays are " << in1 << " and " << in2 << endl << endl;
  }
  if (!in1_is_input) {
    in1_delay = add_delay(in1_delay, find_path(in1));
  }

  if (!in2_is_input) {
    in2_delay = add_delay(in2_delay, find_path(in2));
  }

  return add_delay(max_delay(in1_delay, in2_delay), gate_delay);
}

void get_critical_path() {
  for (string output : outputs) {
    string curr_wire = output;
    float path_delay = find_path(output);
    cout << "Path delay for " << output << " is " << path_delay << endl;
  }
}

int main() {
  read_timings();
  read_benchmark("BENCHMARKS/Combinational1PO", "c17_1PO");
  get_critical_path();

  return 0;
}
