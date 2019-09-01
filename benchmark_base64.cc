#include <cmath>
#include <iostream>

// To fetch these headers:
// git clone --recursive https://github.com/dkorolev/benchmark_base64.git

#include "current/bricks/dflags/dflags.h"
#include "current/bricks/strings/chunk.h"
#include "current/bricks/strings/join.h"
#include "current/bricks/time/chrono.h"
#include "current/bricks/util/base64.h"
#include "current/bricks/util/random.h"
#include "current/bricks/util/singleton.h"

#if defined(_DEBUG) || !defined(NDEBUG)
static constexpr bool ndebug = false;
#else
static constexpr bool ndebug = true;
#endif

DEFINE_uint64(m, 1000, "The size of each block to `base64-{encode/decode}`, in bytes.");
DEFINE_uint64(n, 0, "The number of blocks to benchmark on, keep at zero to derive it from `--m` and `--gb` instead.");
DEFINE_double(gb, ndebug ? 0.1 : 0.01, "If `--n` is not set, the number of (unencoded) gigabytes to run the test on.");
DEFINE_bool(log, false, "Output extra details, to stderr. By default, MB/s will be printed, to stdout.");

namespace implementations {

#define BASE64_PERFTEST_EXPAND_MACRO_IMPL(x) x
#define BASE64_PERFTEST_EXPAND_MACRO(x) BASE64_PERFTEST_EXPAND_MACRO_IMPL(x)
static constexpr int counter_begin = BASE64_PERFTEST_EXPAND_MACRO(__COUNTER__) + 1;

template <int>
struct Name;
template <int>
struct Code;

// The canonical implementation keeps a member `std::string tmp` as the stateful storage.
// This is to enable performance tests against other implementations, that are not using `std::string`-s.

#define IMPLEMENTATION(name)                                                                     \
  static constexpr int index_##name = BASE64_PERFTEST_EXPAND_MACRO(__COUNTER__) - counter_begin; \
  template <>                                                                                    \
  struct Name<index_##name> {                                                                    \
    static char const* HumanReadableName() { return #name; }                                     \
  };                                                                                             \
  template <>                                                                                    \
  struct Code<index_##name>

#include "implementations.h"

static constexpr int total = BASE64_PERFTEST_EXPAND_MACRO(__COUNTER__) - counter_begin;

}  // namespace implementations

template <class IMPL>
void Run(size_t n, std::vector<std::string> const& input, std::vector<std::string> const& golden, size_t total_bytes) {
  IMPL impl;

  if (FLAGS_log) {
    std::cerr << "Testing correctness: ...";
  }

  auto const EXPECT_EQ = [](std::string const& lhs, std::string const& rhs) {
    if (lhs != rhs) {
      std::cerr << "\b\b\bFail: '\n" << lhs << "' != '" << rhs << "'.\n";
      for (size_t i = 0; i < lhs.length(); ++i) {
        std::cerr << "lhs[" << i << "] = " << int(lhs[i]) << std::endl;
      }
      for (size_t i = 0; i < rhs.length(); ++i) {
        std::cerr << "rhs[" << i << "] = " << int(rhs[i]) << std::endl;
      }
      std::exit(-1);
    }
  };
  auto const Base64Encode = [&impl](std::string const& original) { return impl.DoEncode(original); };
  auto const Base64Decode = [&impl](std::string const& encoded) { return impl.DoDecode(encoded); };

  EXPECT_EQ("", Base64Encode(""));
  EXPECT_EQ("Zg==", Base64Encode("f"));
  EXPECT_EQ("Zm8=", Base64Encode("fo"));
  EXPECT_EQ("Zm9v", Base64Encode("foo"));
  EXPECT_EQ("Zm9vYg==", Base64Encode("foob"));
  EXPECT_EQ("Zm9vYmE=", Base64Encode("fooba"));
  EXPECT_EQ("Zm9vYmFy", Base64Encode("foobar"));
  EXPECT_EQ("MDw+", Base64Encode("0<>"));

  EXPECT_EQ("", Base64Decode(""));
  EXPECT_EQ("f", Base64Decode("Zg=="));
  EXPECT_EQ("fo", Base64Decode("Zm8="));
  EXPECT_EQ("foo", Base64Decode("Zm9v"));
  EXPECT_EQ("foob", Base64Decode("Zm9vYg=="));
  EXPECT_EQ("fooba", Base64Decode("Zm9vYmE="));
  EXPECT_EQ("foobar", Base64Decode("Zm9vYmFy"));
  EXPECT_EQ("0<>", Base64Decode("MDw+"));

  EXPECT_EQ("fw==", Base64Encode("\x7f"));
  EXPECT_EQ("gA==", Base64Encode("\x80"));

  EXPECT_EQ("\x7f", Base64Decode("fw=="));
  EXPECT_EQ("\x80", Base64Decode("gA=="));

  std::string all_chars;
  all_chars.resize(256);
  for (int i = 0; i < 256; ++i) {
    all_chars[i] = char(i);
  }
  EXPECT_EQ(all_chars, Base64Decode(Base64Encode(all_chars)));

  for (size_t i = 0; i < n; ++i) {
    std::string const encoded = impl.DoEncode(input[i]);
    if (encoded != golden[i]) {
      std::cerr << "Test failed on encoding.\n";
      std::exit(-1);
    }
    current::strings::Chunk decoded = impl.DoDecode(encoded);
    if (decoded != input[i]) {
      std::cerr << "Test failed on decoding.\n";
      std::exit(-1);
    }
  }
  if (FLAGS_log) {
    std::cerr << "\b\b\bDone.\n";
  }

  double const k = 1.0 * total_bytes;  // Divide by total microseconds, get megabytes per second, (1e6 / 1e6) == 1.

  {
    if (FLAGS_log) {
      std::cerr << "Measuring encoding speed: ...";
    }
    std::chrono::microseconds const t0 = current::time::Now();
    for (size_t i = 0; i < n; ++i) {
      impl.DoEncode(input[i]);
    }
    std::chrono::microseconds const t1 = current::time::Now();
    if (FLAGS_log) {
      std::cerr << "\b\b\bDone.\n";
    }
    std::cout << "Encode: " << std::fixed << std::setprecision(3) << (k / (t1 - t0).count()) << " MB/s." << std::endl;
  }

  {
    if (FLAGS_log) {
      std::cerr << "Measuring decoding speed: ...";
    }
    std::chrono::microseconds const t0 = current::time::Now();
    for (size_t i = 0; i < n; ++i) {
      impl.DoDecode(golden[i]);
    }
    std::chrono::microseconds const t1 = current::time::Now();
    if (FLAGS_log) {
      std::cerr << "\b\b\bDone.\n";
    }
    std::cout << "Decode: " << std::fixed << std::setprecision(3) << (k / (t1 - t0).count()) << " MB/s." << std::endl;
  }
}

struct Registry {
  std::vector<std::string> names;
  std::map<
      std::string,
      std::function<void(
          size_t n, std::vector<std::string> const& input, std::vector<std::string> const& golden, size_t total_bytes)>>
      runners;
  void Add(
      std::string const& name,
      std::function<void(
          size_t n, std::vector<std::string> const& input, std::vector<std::string> const& golden, size_t total_bytes)>
          runner) {
    names.push_back(name);
    runners[name] = runner;
  }
};

template <int I_PLUS_ONE>
struct Registerer : Registerer<I_PLUS_ONE - 1> {
  Registerer() {
    constexpr static int i = I_PLUS_ONE - 1;
    current::Singleton<Registry>().Add(
        implementations::Name<i>::HumanReadableName(),
        [](size_t n,
           std::vector<std::string> const& input,
           std::vector<std::string> const& golden,
           size_t total_bytes) { Run<implementations::Code<i>>(n, input, golden, total_bytes); });
  }
};

template <>
struct Registerer<0> {};

static Registerer<implementations::total> registerer_instance;

DEFINE_string(impl,
              current::Singleton<Registry>().names.front(),
              "An implementation to benchmark: " + current::strings::Join(current::Singleton<Registry>().names, '/') +
                  ".");

int main(int argc, char** argv) {
  ParseDFlags(&argc, &argv);

  if (!ndebug) {
    std::cout << "WARNING: You are benchmarking a debug build. `NDEBUG=1 make` on *nix would do the job." << std::endl;
  }

  Registry const& registry = current::Singleton<Registry>();
  auto const impl_cit = registry.runners.find(FLAGS_impl);
  if (impl_cit == registry.runners.end()) {
    std::cout << "ERROR: invalid value provided for `--impl`." << std::endl;
    std::exit(-1);
  }

  size_t const m = FLAGS_m > 0 ? static_cast<size_t>(FLAGS_m) : 100;
  size_t const n = FLAGS_n ? static_cast<size_t>(FLAGS_n) : static_cast<size_t>(std::ceil(1e9 * FLAGS_gb / m));

  if (FLAGS_log) {
    std::cerr << "Generating " << n << " strings of length " << m << " characters each: ...";
  }

  std::vector<std::string> input(n);
  for (std::string& s : input) {
    s.resize(m);
    for (char& c : s) {
      c = static_cast<char>(current::random::RandomInt(32, 127));
    }
  }

  if (FLAGS_log) {
    std::cerr << "\b\b\bDone.\n";
  }

  if (FLAGS_log) {
    std::cerr << "Preparing golden results for them: ...";
  }

  std::vector<std::string> golden(n);
  for (size_t i = 0; i < n; ++i) {
    golden[i] = current::Base64Encode(input[i]);
  }

  if (FLAGS_log) {
    std::cerr << "\b\b\bDone.\n";
  }

  impl_cit->second(input.size(), input, golden, n * m);
}
