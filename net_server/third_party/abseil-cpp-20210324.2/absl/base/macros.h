//
// Copyright 2017 The Abseil Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// -----------------------------------------------------------------------------
// File: macros.h
// -----------------------------------------------------------------------------
//
// This header file defines the set of language macros used within Abseil code.
// For the set of macros used to determine supported compilers and platforms,
// see absl/base/config.h instead.
//
// This code is compiled directly on many platforms, including client
// platforms like Windows, Mac, and embedded systems.  Before making
// any changes here, make sure that you're not breaking any platforms.

#ifndef ABSL_BASE_MACROS_H_
#define ABSL_BASE_MACROS_H_

#include <cassert>
#include <cstddef>

#include "absl/base/attributes.h"
#include "absl/base/config.h"
#include "absl/base/optimization.h"
#include "absl/base/port.h"

// ABSL_ARRAYSIZE()
//
// Returns the number of elements in an array as a compile-time constant, which
// can be used in defining new arrays. If you use this macro on a pointer by
// mistake, you will get a compile-time error.
#define ABSL_ARRAYSIZE(array) \
  (sizeof(::absl::macros_internal::ArraySizeHelper(array)))

namespace absl {
ABSL_NAMESPACE_BEGIN
namespace macros_internal {
// Note: this internal template function declaration is used by ABSL_ARRAYSIZE.
// The function doesn't need a definition, as we only use its type.
template <typename T, size_t N>
auto ArraySizeHelper(const T (&array)[N]) -> char (&)[N];
}  // namespace macros_internal
ABSL_NAMESPACE_END
}  // namespace absl

// ABSL_ASSERT()
//
// In C++11, `assert` can't be used portably within constexpr functions.
// ABSL_ASSERT functions as a runtime assert but works in C++11 constexpr
// functions.  Example:
//
// constexpr double Divide(double a, double b) {
//   return ABSL_ASSERT(b != 0), a / b;
// }
//
// This macro is inspired by
// https://akrzemi1.wordpress.com/2017/05/18/asserts-in-constexpr-functions/
#if defined(NDEBUG)
#define ABSL_ASSERT(expr) \
  (false ? static_cast<void>(expr) : static_cast<void>(0))
#else
#define ABSL_ASSERT(expr)                           \
  (ABSL_PREDICT_TRUE((expr)) ? static_cast<void>(0) \
                             : [] { assert(false && #expr); }())  // NOLINT
#endif

// `ABSL_INTERNAL_HARDENING_ABORT()` controls how `ABSL_HARDENING_ASSERT()`
// aborts the program in release mode (when NDEBUG is defined). The
// implementation should abort the program as quickly as possible and ideally it
// should not be possible to ignore the abort request.
#if (ABSL_HAVE_BUILTIN(__builtin_trap) &&         \
     ABSL_HAVE_BUILTIN(__builtin_unreachable)) || \
    (defined(__GNUC__) && !defined(__clang__))
#define ABSL_INTERNAL_HARDENING_ABORT() \
  do {                                  \
    __builtin_trap();                   \
    __builtin_unreachable();            \
  } while (false)
#else
#define ABSL_INTERNAL_HARDENING_ABORT() abort()
#endif

// ABSL_HARDENING_ASSERT()
//
// `ABSL_HARDENING_ASSERT()` is like `ABSL_ASSERT()`, but used to implement
// runtime assertions that should be enabled in hardened builds even when
// `NDEBUG` is defined.
//
// When `NDEBUG` is not defined, `ABSL_HARDENING_ASSERT()` is identical to
// `ABSL_ASSERT()`.
//
// See `ABSL_OPTION_HARDENED` in `absl/base/options.h` for more information on
// hardened mode.
#if ABSL_OPTION_HARDENED == 1 && defined(NDEBUG)
#define ABSL_HARDENING_ASSERT(expr)                 \
  (ABSL_PREDICT_TRUE((expr)) ? static_cast<void>(0) \
                             : [] { ABSL_INTERNAL_HARDENING_ABORT(); }())
#else
#define ABSL_HARDENING_ASSERT(expr) ABSL_ASSERT(expr)
#endif

#ifdef ABSL_HAVE_EXCEPTIONS
#define ABSL_INTERNAL_TRY try
#define ABSL_INTERNAL_CATCH_ANY catch (...)
#define ABSL_INTERNAL_RETHROW do { throw; } while (false)
#else  // ABSL_HAVE_EXCEPTIONS
#define ABSL_INTERNAL_TRY if (true)
#define ABSL_INTERNAL_CATCH_ANY else if (false)
#define ABSL_INTERNAL_RETHROW do {} while (false)
#endif  // ABSL_HAVE_EXCEPTIONS

// `ABSL_INTERNAL_UNREACHABLE` is an unreachable statement.  A program which
// reaches one has undefined behavior, and the compiler may optimize
// accordingly.
#if defined(__GNUC__) || ABSL_HAVE_BUILTIN(__builtin_unreachable)
#define ABSL_INTERNAL_UNREACHABLE __builtin_unreachable()
#elif defined(_MSC_VER)
#define ABSL_INTERNAL_UNREACHABLE __assume(0)
#else
#define ABSL_INTERNAL_UNREACHABLE
#endif

#endif  // ABSL_BASE_MACROS_H_
