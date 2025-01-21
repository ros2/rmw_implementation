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

#include <gtest/gtest.h>

#include "rmw/error_handling.h"
#include "rmw/rmw.h"

TEST(TestPublisherAllocator, init_fini_publisher_allocation)
{
  rmw_ret_t ret = rmw_init_publisher_allocation(nullptr, nullptr, nullptr);
  rmw_reset_error();
  if (ret == RMW_RET_UNSUPPORTED) {
    GTEST_SKIP();
  }

  ASSERT_NE(ret, RMW_RET_UNSUPPORTED);

  // TODO(anyone): Add tests here when the API has been implemented
}
