#ifndef __FILE_PARSE_H__
#define __FILE_PARSE_H__

#include <map>
#include <memory>
#include <string>
#include <vector>

typedef struct Timing {
  std::string gate;
  float cost;
  std::vector<float> delays;
} Timing;

struct _Gate;
typedef struct _Gate Gate;

typedef struct Wire {
  std::string name;
  std::vector<std::shared_ptr<Gate>> gates;
  std::map<std::string, std::vector<float>> delay_vars;
} Wire;

struct _Gate {
  std::string type;
  Timing *strength;
  std::vector<Wire *> inputs;
  std::vector<Wire *> outputs;

  _Gate(std::string type, Timing *strength, std::vector<Wire *> inputs,
        std::vector<Wire *> outputs)
      : type(type), strength(strength), inputs(inputs), outputs(outputs) {}
};

typedef struct Reverse_Gate {
  // Gate *gate;
  std::shared_ptr<Gate> gate;
  std::vector<std::string> inputs;
} Reverse_Gate;

extern int DEBUG;
extern std::map<std::string, Timing> lib_time;
extern std::map<std::string, Wire> wires;
extern std::vector<std::string> inputs;
extern std::vector<std::string> outputs;
extern std::map<std::string, std::vector<Gate>> gates;
extern std::map<std::string, Reverse_Gate> reverse_gates;

int read_timings();
int read_benchmark(std::string, std::string);

#endif
