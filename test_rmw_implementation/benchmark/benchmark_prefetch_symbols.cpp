// Copyright 2020 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <string>

#include "performance_test_fixture/performance_test_fixture.hpp"

#include "rcutils/strdup.h"

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

using performance_test_fixture::PerformanceTest;

BENCHMARK_DEFINE_F(PerformanceTest, prefetch_symbols)(benchmark::State & st)
{
  rmw_init_options_t options;
  rmw_context_t context;

  options = rmw_get_zero_initialized_init_options();
  rmw_ret_t ret = rmw_init_options_init(&options, rcutils_get_default_allocator());
  if (ret != RMW_RET_OK) {
    st.SkipWithError("rmw_init_options_init failed");
  }
  options.enclave = rcutils_strdup("/", rcutils_get_default_allocator());

  for (auto _ : st) {
    /// rmw_init expects a zero initialized context
    context = rmw_get_zero_initialized_context();
    ret = rmw_init(&options, &context);
    if (ret != RMW_RET_OK) {
      st.SkipWithError(rcutils_get_error_string().str);
    }
  }

  ret = rmw_shutdown(&context);
  if (ret != RMW_RET_OK) {
    st.SkipWithError("rmw_shutdown failed");
  }
  ret = rmw_context_fini(&context);
  if (ret != RMW_RET_OK) {
    st.SkipWithError("rmw_context_fini failed");
  }
  ret = rmw_init_options_fini(&options);
  if (ret != RMW_RET_OK) {
    st.SkipWithError("rmw_init_options_fini failed");
  }
}
BENCHMARK_REGISTER_F(PerformanceTest, prefetch_symbols);
