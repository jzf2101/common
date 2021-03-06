#pragma once

#include <microscopes/common/random_fwd.hpp>
#include <microscopes/common/macros.hpp>
#include <distributions/random.hpp>

#include <google/protobuf/message.h>

#include <eigen3/Eigen/Dense>
#include <eigen3/Eigen/Cholesky>

#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <utility>
#include <random>
#include <algorithm>

// pretty printer for std::pair<A, B>
template <typename A, typename B>
static inline std::ostream &
operator<<(std::ostream &o, const std::pair<A, B> &p)
{
  o << "[" << p.first << ", " << p.second << "]";
  return o;
}

// pretty printer for std::vector<T, Alloc>
template <typename T, typename Alloc>
static inline std::ostream &
operator<<(std::ostream &o, const std::vector<T, Alloc> &v)
{
  bool first = true;
  o << "[";
  for (const auto &p : v) {
    if (!first)
      o << ", ";
    first = false;
    o << p;
  }
  o << "]";
  return o;
}

template <typename Key, typename Value, typename Cmp, typename Alloc>
static inline std::ostream &
operator<<(std::ostream &o, const std::map<Key, Value, Cmp, Alloc> &m)
{
  bool first = true;
  o << "{";
  for (const auto &p : m) {
    if (!first)
      o << ", ";
    first = false;
    o << p.first << " => " << p.second;
  }
  o << "}";
  return o;
}

namespace microscopes {
namespace common {

struct util {

  static inline void
  inplace_range(std::vector<size_t> &a, size_t n)
  {
    a.clear();
    a.reserve(n);
    for (size_t i = 0; i < n; i++)
      a.push_back(i);
  }

  static inline std::vector<size_t>
  range(size_t n)
  {
    std::vector<size_t> ret;
    inplace_range(ret, n);
    return ret;
  }

  // generate a random permutation of the integers [0, ..., n-1]
  static inline void
  inplace_permute(std::vector<size_t> &pi, size_t n, rng_t &rng)
  {
    inplace_range(pi, n);
    for (size_t i = pi.size() - 1; i >= 1; i--) {
      std::uniform_int_distribution<size_t> dist(0, i);
      const size_t j = dist(rng);
      std::swap(pi[j], pi[i]);
    }
  }

  static inline std::vector<size_t>
  permute(size_t n, rng_t &rng)
  {
    std::vector<size_t> ret;
    inplace_permute(ret, n, rng);
    return ret;
  }

  /**
   * guaranteed to produce contiguous groups
   * (so taking the max() + 1 is the total # of groups)
   */
  static inline std::vector<size_t>
  random_assignment_vector(size_t n, rng_t &rng, size_t maxgroups=100)
  {
    std::vector<size_t> ret(n);
    // create min(maxgroups, n) groups
    size_t ngroups = std::min(size_t(maxgroups), n);
    const auto groups = range(ngroups);
    for (size_t i = 0; i < n; i++) {
      const auto choice = sample_choice(groups, rng);
      ret[i] = choice;
    }
    return ret;
  }

  // copy from:
  // https://github.com/forcedotcom/distributions/blob/master/distributions/util.py

  static inline void
  scores_to_probs(std::vector<float> &scores)
  {
    const float m = *std::max_element(scores.begin(), scores.end());
    for (auto &s : scores)
      s -= m;
    for (auto &s : scores)
      s = expf(s);
    const float acc = std::accumulate(scores.begin(), scores.end(), 0.);
    for (auto &s : scores)
      s /= acc;
  }

  static inline size_t
  sample_discrete_log(std::vector<float> &scores, rng_t &rng)
  {
    scores_to_probs(scores);
    return sample_discrete(scores, rng);
  }

  static inline size_t
  sample_discrete(const std::vector<float> &probs, rng_t &rng)
  {
    // assumes probs add up to 1
    float dart = distributions::sample_unif01(rng);
    for (size_t i = 0; i < probs.size(); i++) {
      dart -= probs[i];
      if (dart <= 0.)
        return i;
    }
    return probs.size() - 1;
  }

  template <typename T>
  static inline T
  sample_choice(const std::vector<T> &choices, rng_t &rng)
  {
    float prob = 1./float(choices.size());
    const std::vector<float> probs(choices.size(), prob);
    return choices[sample_discrete(probs, rng)];
  }

  static inline std::string
  hexify(const uint8_t *buf, size_t n)
  {
    const char *table = "0123456789abcdef";
    std::string res(n * 2, '0');
    for (size_t i = 0; i < n; i++) {
      res[i*2]   = table[buf[i] / 16];
      res[i*2+1] = table[buf[i] % 16];
    }
    return res;
  }

  template <typename T>
  static inline std::string
  to_string(const T &t)
  {
    std::ostringstream oss;
    oss << t;
    return oss.str();
  }

  static inline bool
  is_symmetric_positive_definite(const Eigen::MatrixXf &m)
  {
    if (!m.isApprox(m.transpose()))
      return false;
    Eigen::LDLT<Eigen::MatrixXf> ldlt;
    ldlt.compute(m);
    return ldlt.isPositive();
  }

  template <typename T>
  static inline void
  remove_row(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> &m, size_t row_to_remove)
  {
      size_t num_rows = m.rows()-1;
      size_t num_cols = m.cols();

      if( row_to_remove < num_rows )
          m.block(row_to_remove,0,num_rows-row_to_remove,num_cols) = m.block(row_to_remove+1,0,num_rows-row_to_remove,num_cols);

      m.conservativeResize(num_rows,num_cols);
  }

  template <typename T>
  static inline void
  remove_column(Eigen::Matrix<T, Eigen::Dynamic, Eigen::Dynamic> &m, size_t col_to_remove)
  {
      size_t num_rows = m.rows();
      size_t num_cols = m.cols()-1;

      if( col_to_remove < num_cols )
          m.block(0,col_to_remove,num_rows,num_cols-col_to_remove) = m.block(0,col_to_remove+1,num_rows,num_cols-col_to_remove);

      m.conservativeResize(num_rows,num_cols);
  }

  // helpers for python
  static inline ALWAYS_INLINE void
  set(Eigen::MatrixXf &m, unsigned i, unsigned j, float v)
  {
    m(i, j) = v;
  }

  static inline ALWAYS_INLINE float
  get(const Eigen::MatrixXf &m, unsigned i, unsigned j)
  {
    return m(i, j);
  }

  static inline std::string
  protobuf_to_string(const google::protobuf::Message &m)
  {
    std::ostringstream out;
    m.SerializeToOstream(&out);
    return out.str();
  }

  static inline void
  protobuf_from_string(google::protobuf::Message &m, const std::string &s)
  {
    std::istringstream inp(s);
    m.ParseFromIstream(&inp);
  }

};

} // namespace common
} // namespace microscopes
