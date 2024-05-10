#include "math_functions.hpp"

#include <cmath>
#include <iostream>
#include <vector>

using namespace std;

#define _USE_MATH_DEFINES

constexpr float SMALLPHI_COEFF = 1 / sqrt(2 * M_PI);

float calc_delay(vector<float> &a) { return a[0] + standard_deviation(a); }

vector<float> add(vector<float> &a, vector<float> &b) {
  vector<float> sum;
  sum.push_back(a[0] + b[0]);
  sum.push_back(a[1] + b[1]);
  sum.push_back(a[2] + b[2]);
  sum.push_back(sqrt(a[3] * a[3] + b[3] * b[3]));

  return sum;
}

vector<float> maximum(vector<float> &a, vector<float> &b) {
  float sigmaA = standard_deviation(a);
  float sigmaB = standard_deviation(b);
  float rho = correlation_coefficient(a, b, sigmaA, sigmaB);
  float theta = combined_standard_deviation(sigmaA, sigmaB, rho);
  float x = normalize_difference(a, b, theta);
  float phi = cumulative_distribution_function(x);
  float phiX = probability_density_function(x);
  float mean = expected_max_value(a, b, phi, theta, phiX);
  float variance =
      variance_of_max(a, b, sigmaA, sigmaB, theta, phi, phiX, mean);

  vector<float> result;
  result.push_back(mean);
  result.push_back(a[1] * phi + b[1] * (1.0 - phi));
  result.push_back(a[2] * phi + b[2] * (1.0 - phi));
  result.push_back(sqrt(max(0.0f, variance)));
  return result;
}

vector<float> minimum(vector<float> &a, vector<float> &b) {
  float sigmaA = standard_deviation(a);
  float sigmaB = standard_deviation(b);
  float rho = correlation_coefficient(a, b, sigmaA, sigmaB);
  float theta = combined_standard_deviation(sigmaA, sigmaB, rho);
  float x = normalize_difference(a, b, theta);
  float phi = cumulative_distribution_function(x);
  float phiX = probability_density_function(x);
  float mean = expected_min_value(a, b, phi, theta, phiX);
  float variance =
      variance_of_min(a, b, sigmaA, sigmaB, theta, phi, phiX, mean);

  vector<float> result;
  result.push_back(mean);
  result.push_back(a[1] * (1.0 - phi) + b[1] * phi);
  result.push_back(a[2] * (1.0 - phi) + b[2] * phi);
  result.push_back(sqrt(max(0.0f, variance)));
  return result;
}

float standard_deviation(vector<float> &a) {
  return sqrt(a[1] * a[1] + a[2] * a[2] + a[3] * a[3]);
}

float correlation_coefficient(vector<float> &a, vector<float> &b, float sigmaA,
                              float sigmaB) {
  return (a[1] * b[1] + a[2] * b[2]) / (sigmaA * sigmaB);
}

float combined_standard_deviation(float sigmaA, float sigmaB, float rho) {
  return sqrt(sigmaA * sigmaA + sigmaB * sigmaB - 2 * sigmaA * sigmaB * rho);
}

float normalize_difference(vector<float> &a, vector<float> &b, float theta) {
  return (a[0] - b[0]) / theta;
}

float cumulative_distribution_function(float x) {
  return 0.5 * (1.0 + erf(x / sqrt(2.0)));
}

float probability_density_function(float x) {
  return SMALLPHI_COEFF * exp(-0.5 * x * x);
}

float expected_max_value(vector<float> &a, vector<float> &b, float phi,
                         float theta, float phiX) {
  return a[0] * phi + b[0] * (1 - phi) + theta * phiX;
}

float expected_min_value(vector<float> &a, vector<float> &b, float phi,
                         float theta, float phiX) {
  return a[0] * (1 - phi) + b[0] * phi - theta * phiX;
}

float variance_of_max(vector<float> &a, vector<float> &b, float sigmaA,
                      float sigmaB, float theta, float phi, float phiX,
                      float mean) {
  return (sigmaA * sigmaA + a[0] * a[0]) * phi +
         (sigmaB * sigmaB + b[0] * b[0]) * (1 - phi) +
         (a[0] + b[0]) * theta * phiX - mean * mean;
}

float variance_of_min(vector<float> &a, vector<float> &b, float sigmaA,
                      float sigmaB, float theta, float phi, float phiX,
                      float mean) {
  return (sigmaA * sigmaA + a[0] * a[0]) * (1 - phi) +
         (sigmaB * sigmaB + b[0] * b[0]) * phi - (a[0] + b[0]) * theta * phiX -
         mean * mean;
}
