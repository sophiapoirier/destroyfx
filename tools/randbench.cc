
#include "dfxmath.h"

#include <cstdint>
#include <chrono>
#include <stdio.h>

using namespace std;

int main(int argc, char **argv) {

  {
    dfx::math::RandomGenerator<double> rgd(dfx::math::RandomSeed::Static,
                                           1.0, 5.0);
    printf("Random doubles:\n");
    printf("%.6f\n", rgd.next());
    printf("%.6f\n", rgd.next());
    printf("%.6f\n", rgd.next());
    printf("%.6f\n", rgd.next());     
  }

  
  constexpr int SIZE = 128;
  
  dfx::math::RandomGenerator<int> rg(dfx::math::RandomSeed::Static,
                                     0, SIZE);
  
  vector<int64_t> histo(SIZE, 0LL);
  const int64_t ITERS = 1'000'000'000LL;

  const auto time_start = std::chrono::steady_clock::now();
  for (int iter = 0; iter < ITERS; iter++) {
    int sample = rg.next();
    histo[sample]++;
  }
    
  const auto time_now = std::chrono::steady_clock::now();
  const std::chrono::duration<double> time_elapsed =
    time_now - time_start;
  const double seconds = time_elapsed.count();

  int64_t m = 0;
  for (int64_t v : histo) m = std::max(m, v);
  
  for (int i = 0; i < SIZE; i++) {
    printf("%03d % 6lld |", i, histo[i]);
    int stars = std::round((50.0 * histo[i]) / m);
    while (stars--) printf("*");
    printf("\n");
  }
  
  printf("%lld iters in %.4f sec\n", ITERS, seconds);
  printf("sizeof rg: %llu\n", sizeof (rg));
  
  return 0;
}
