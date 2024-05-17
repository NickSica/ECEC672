#include "file_parse.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string_regex.hpp>
#include <fstream>
#include <iostream>
#include <map>
#include <memory>
#include <string>
#include <vector>

using namespace std;
using namespace boost::algorithm;

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

// std::map<std::string, Timing> lib_time;
// std::map<std::string, Wire> wires;
// std::vector<std::string> inputs;
// std::vector<std::string> outputs;
// std::map<std::string, std::vector<Gate>> gates;
// std::map<std::string, Reverse_Gate> reverse_gates;

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
    Timing t = {gate, cost, {}};
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
        wires[start_wire].name = start_wire;
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
      gates[gate].emplace_back(gate, &lib_time[gate + "1"], gate_in, gate_out);
      std::shared_ptr<Gate> gate_ptr = make_shared<Gate>(gates[gate].back());

      for (auto input : inputs) {
        wires[input].gates.push_back(gate_ptr);
      }

      Reverse_Gate r_gate = {gate_ptr, inputs};
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
