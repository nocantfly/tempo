#include "SimpleKalmanFilter.h"
#include "math.h"

SimpleKalmanFilter::SimpleKalmanFilter(float mea_e, float est_e, float q) {
  _err_measure = mea_e;
  _err_estimate = est_e;
  _q = q;
}

void SimpleKalmanFilter::reset(float est) {
  _current_estimate = est;
  _last_estimate = est;
  _kalman_gain = 0.0f;
  _err_estimate = _err_measure;
}

float SimpleKalmanFilter::updateEstimate(float mea) {
  _kalman_gain = _err_estimate / (_err_estimate + _err_measure);
  _current_estimate = _last_estimate + _kalman_gain * (mea - _last_estimate);
  _err_estimate = (1.0f - _kalman_gain) * _err_estimate + fabsf(_last_estimate - _current_estimate) * _q;
  _last_estimate = _current_estimate;
  return _current_estimate;
}
