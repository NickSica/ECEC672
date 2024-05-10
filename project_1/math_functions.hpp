#ifndef __MATH_FNS_H__
#define __MATH_FNS_H__

#include <vector>

float calc_delay(std::vector<float> &a);
std::vector<float> add(std::vector<float> &a, std::vector<float> &b);
int maximum(std::vector<float> *a, std::vector<float> *b);
// std::vector<float> maximum(std::vector<float> &a, std::vector<float> &b);
std::vector<float> minimum(std::vector<float> &a, std::vector<float> &b);
float standard_deviation(std::vector<float> &a);
float correlation_coefficient(std::vector<float> &a, std::vector<float> &b,
                              float sigmaA, float sigmaB);
float combined_standard_deviation(float sigmaA, float sigmaB, float rho);
float normalize_difference(std::vector<float> &a, std::vector<float> &b,
                           float theta);
float cumulative_distribution_function(float x);
float probability_density_function(float x);
float expected_max_value(std::vector<float> &a, std::vector<float> &b,
                         float phi, float theta, float phiX);
float expected_min_value(std::vector<float> &a, std::vector<float> &b,
                         float phi, float theta, float phiX);
float variance_of_max(std::vector<float> &a, std::vector<float> &b,
                      float sigmaA, float sigmaB, float theta, float phi,
                      float phiX, float mean);
float variance_of_min(std::vector<float> &a, std::vector<float> &b,
                      float sigmaA, float sigmaB, float theta, float phi,
                      float phiX, float mean);

#endif
