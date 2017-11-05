#ifndef preproc_Utilities_pairUtilities_hh
#define preproc_Utilities_pairUtilities_hh

// Utility functions/classes specifically for use with pairs and containers of pairs.

#include <stdio.h>
#include <utility>

namespace util {

  struct pair_add {
    template<typename T1, typename T2>
    T1 operator()(T1 x, const std::pair<T2,T1>& y){
      return x + y.second;
    }
  };

  struct pair_comp {
    template<typename T1, typename T2>
    bool operator()(const std::pair<T1,T2>& x,
		    const std::pair<T1,T2>& y){
      return x.second < y.second;
    }
  };

  /// To help with precision in the waveform map.
  /// Do not like this solution but nothing else seems to work.
  // class key_d_comp {
  // private:
  //   double epsilon;
  // public:
  //   key_d_comp(double epsi = 0.000001): epsilon(epsi){}
  //   bool operator()(const double& lhv,
  // 		    const double& rhv) const {
  //     return lhv < rhv - epsilon;
  //   }
  // };

  class key_f_comp {
  private:
    float epsilon;
  public:
    key_f_comp(float epsi = 0.000001): epsilon(epsi){}
    bool operator()(const float& lhv,
		    const float& rhv) const {
      return lhv < rhv - epsilon;
    }
  };

} // numUtil namespace

#endif
