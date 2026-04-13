#ifndef SIMPLE_KALMAN_FILTER_H
#define SIMPLE_KALMAN_FILTER_H

class SimpleKalmanFilter {
public:
  SimpleKalmanFilter(float mea_e, float est_e, float q);
  float updateEstimate(float mea);
  void setEstimate(float est) { _current_estimate = est; _last_estimate = est; }
  void reset(float est = 0.0f);

private:
  float _err_measure;
  float _err_estimate;
  float _q;
  float _current_estimate = 0;
  float _last_estimate = 0;
  float _kalman_gain = 0;
};
#endif
