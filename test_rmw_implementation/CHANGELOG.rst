^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package test_rmw_implementation
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

2.15.6 (2025-08-05)
-------------------
* Test failing deserialization of invalid sequence length (`#261 <https://github.com/ros2/rmw_implementation/issues/261>`_) (`#263 <https://github.com/ros2/rmw_implementation/issues/263>`_)
  * Add test infrastructure.
  * Test that deserialization with wrong sequence length fails.
  ---------
  (cherry picked from commit 4dd5d571a5bfa1a67183acf271dfa442932c7572)
  Co-authored-by: Miguel Company <miguelcompany@eprosima.com>
* add ignore_local_publications_serialized test. (`#255 <https://github.com/ros2/rmw_implementation/issues/255>`_) (`#257 <https://github.com/ros2/rmw_implementation/issues/257>`_)
  (cherry picked from commit 1eceed45fbdbe6b93bb49993f1bed9698aeca38f)
  Co-authored-by: Tomoya Fujita <Tomoya.Fujita@sony.com>
* Contributors: mergify[bot]

2.15.5 (2025-03-12)
-------------------
* Added rmw_event_type_is_supported (`#250 <https://github.com/ros2/rmw_implementation/issues/250>`_) (`#252 <https://github.com/ros2/rmw_implementation/issues/252>`_)
* Update expectations of tests to remain compatible with non-DDS middlewares (`#248 <https://github.com/ros2/rmw_implementation/issues/248>`_) (`#251 <https://github.com/ros2/rmw_implementation/issues/251>`_)
* Contributors: Alejandro Hernández Cordero, mergify[bot]

2.15.4 (2024-12-18)
-------------------

2.15.3 (2024-06-27)
-------------------
* Add test creating two content filter topics with the same topic name (`#230 <https://github.com/ros2/rmw_implementation/issues/230>`_) (`#233 <https://github.com/ros2/rmw_implementation/issues/233>`_) (`#237 <https://github.com/ros2/rmw_implementation/issues/237>`_)
  Co-authored-by: Mario Domínguez López <116071334+Mario-DL@users.noreply.github.com>
  (cherry picked from commit 16e14d15e210672fbfe0beb1f57effbd8d1233b0)
  Co-authored-by: Alejandro Hernández Cordero <ahcorde@gmail.com>
* Contributors: mergify[bot]

2.15.2 (2024-04-24)
-------------------

2.15.1 (2024-03-28)
-------------------
* Compile the test_rmw_implementation tests fewer times. (`#224 <https://github.com/ros2/rmw_implementation/issues/224>`_)
* Contributors: Chris Lalancette

2.15.0 (2023-12-26)
-------------------
* Switch to using target_link_libraries everywhere. (`#222 <https://github.com/ros2/rmw_implementation/issues/222>`_)
* Contributors: Chris Lalancette

2.14.0 (2023-10-04)
-------------------
* Add rmw_count_clients,services & test (`#208 <https://github.com/ros2/rmw_implementation/issues/208>`_)
* Contributors: Minju, Lee

2.13.0 (2023-04-27)
-------------------

2.12.0 (2023-04-11)
-------------------
* Add tests for rmw matched event (`#216 <https://github.com/ros2/rmw_implementation/issues/216>`_)
* Contributors: Barry Xu

2.11.0 (2023-02-13)
-------------------
* Update rmw_implementation to C++17. (`#214 <https://github.com/ros2/rmw_implementation/issues/214>`_)
* [rolling] Update maintainers - 2022-11-07 (`#212 <https://github.com/ros2/rmw_implementation/issues/212>`_)
* Contributors: Audrow Nash, Chris Lalancette

2.10.0 (2022-11-02)
-------------------
* Add rmw_get_gid_for_client & tests (`#206 <https://github.com/ros2/rmw_implementation/issues/206>`_)
* Contributors: Brian

2.9.1 (2022-09-13)
------------------

2.9.0 (2022-04-29)
------------------

2.8.1 (2022-03-28)
------------------
* add content-filtered-topic interfaces (`#181 <https://github.com/ros2/rmw_implementation/issues/181>`_)
* Contributors: Chen Lihui

2.8.0 (2022-03-01)
------------------

2.7.1 (2022-01-14)
------------------
* Fix linter issues (`#200 <https://github.com/ros2/rmw_implementation/issues/200>`_)
* Contributors: Jorge Perez

2.7.0 (2021-11-19)
------------------
* Add client/service QoS getters. (`#196 <https://github.com/ros2/rmw_implementation/issues/196>`_)
* Contributors: mauropasse

2.6.1 (2021-11-18)
------------------
* Added tests for bounded sequences serialization (`#193 <https://github.com/ros2/rmw_implementation/issues/193>`_)
* Contributors: Miguel Company

2.6.0 (2021-08-09)
------------------
* Add RMW_DURATION_INFINITE basic compliance test. (`#194 <https://github.com/ros2/rmw_implementation/issues/194>`_)
* Test SubscriptionOptions::ignore_local_publications. (`#192 <https://github.com/ros2/rmw_implementation/issues/192>`_)
* Add rmw_publisher_wait_for_all_acked. (`#188 <https://github.com/ros2/rmw_implementation/issues/188>`_)
* Wait for server in test_rmw_implementation service tests. (`#191 <https://github.com/ros2/rmw_implementation/issues/191>`_)
* Contributors: Barry Xu, Emerson Knapp, Jose Antonio Moral, Michel Hidalgo

2.5.0 (2021-05-05)
------------------

2.4.1 (2021-04-16)
------------------
* Implement test for subscription loaned messages (`#186 <https://github.com/ros2/rmw_implementation/issues/186>`_)
* Contributors: Miguel Company

2.4.0 (2021-04-06)
------------------

2.3.0 (2021-03-25)
------------------
* Remove rmw_connext_cpp. (`#183 <https://github.com/ros2/rmw_implementation/issues/183>`_)
* Add support for rmw_connextdds (`#182 <https://github.com/ros2/rmw_implementation/issues/182>`_)
* Contributors: Andrea Sorbini, Chris Lalancette

2.2.0 (2021-03-08)
------------------
* Add function for checking QoS profile compatibility (`#180 <https://github.com/ros2/rmw_implementation/issues/180>`_)
* Make sure to initialize the rmw_message_sequence after init. (`#175 <https://github.com/ros2/rmw_implementation/issues/175>`_)
* Set the value of is_available before entering the loop (`#173 <https://github.com/ros2/rmw_implementation/issues/173>`_)
* Contributors: Chris Lalancette, Jacob Perron

2.1.2 (2021-01-29)
------------------
* Set the return value of rmw_ret_t before entering the loop. (`#171 <https://github.com/ros2/rmw_implementation/issues/171>`_)
* Contributors: Chris Lalancette

2.1.1 (2021-01-25)
------------------

2.1.0 (2020-12-10)
------------------
* Add some additional checking that cleanup happens. (`#168 <https://github.com/ros2/rmw_implementation/issues/168>`_)
* Add test to check rmw_send_response when the client is gone (`#162 <https://github.com/ros2/rmw_implementation/issues/162>`_)
* Update maintainers (`#154 <https://github.com/ros2/rmw_implementation/issues/154>`_)
* Add fault injection tests to construction/destroy APIs.  (`#144 <https://github.com/ros2/rmw_implementation/issues/144>`_)
* Add tests bad type_support implementation (`#152 <https://github.com/ros2/rmw_implementation/issues/152>`_)
* Add tests for localhost-only node creation (`#150 <https://github.com/ros2/rmw_implementation/issues/150>`_)
* Added rmw_service_server_is_available tests (`#140 <https://github.com/ros2/rmw_implementation/issues/140>`_)
* Use 10x the intraprocess delay to wait for sent requests. (`#148 <https://github.com/ros2/rmw_implementation/issues/148>`_)
* Added rmw_wait, rmw_create_wait_set, and rmw_destroy_wait_set tests (`#139 <https://github.com/ros2/rmw_implementation/issues/139>`_)
* Add tests service/client request/response with bad arguments (`#141 <https://github.com/ros2/rmw_implementation/issues/141>`_)
* Added test for rmw_get_serialized_message_size (`#142 <https://github.com/ros2/rmw_implementation/issues/142>`_)
* Add service/client construction/destruction API test coverage. (`#138 <https://github.com/ros2/rmw_implementation/issues/138>`_)
* Added rmw_publisher_allocation and rmw_subscription_allocation related tests (`#137 <https://github.com/ros2/rmw_implementation/issues/137>`_)
* Add tests take serialized with info bad arguments (`#130 <https://github.com/ros2/rmw_implementation/issues/130>`_)
* Add gid API test coverage. (`#134 <https://github.com/ros2/rmw_implementation/issues/134>`_)
* Add tests take bad arguments  (`#125 <https://github.com/ros2/rmw_implementation/issues/125>`_)
* Bump graph API test coverage. (`#132 <https://github.com/ros2/rmw_implementation/issues/132>`_)
* Add tests take sequence serialized with bad arguments (`#129 <https://github.com/ros2/rmw_implementation/issues/129>`_)
* Add tests take sequence + take sequence with bad arguments (`#128 <https://github.com/ros2/rmw_implementation/issues/128>`_)
* Add tests take with info bad arguments (`#126 <https://github.com/ros2/rmw_implementation/issues/126>`_)
* Add tests for non-implemented rmw_take\_* functions (`#131 <https://github.com/ros2/rmw_implementation/issues/131>`_)
* Add tests publish serialized bad arguments (`#124 <https://github.com/ros2/rmw_implementation/issues/124>`_)
* Add tests publish bad arguments (`#123 <https://github.com/ros2/rmw_implementation/issues/123>`_)
* Add tests non-implemented functions + loan bad arguments (`#122 <https://github.com/ros2/rmw_implementation/issues/122>`_)
* Add missing empty topic name tests. (`#136 <https://github.com/ros2/rmw_implementation/issues/136>`_)
* Add rmw_get_serialization_format() smoke test. (`#133 <https://github.com/ros2/rmw_implementation/issues/133>`_)
* Complete publisher/subscription QoS query API test coverage. (`#120 <https://github.com/ros2/rmw_implementation/issues/120>`_)
* Remove duplicate assertions (`#121 <https://github.com/ros2/rmw_implementation/issues/121>`_)
* Add publisher/subscription matched count API test coverage. (`#119 <https://github.com/ros2/rmw_implementation/issues/119>`_)
* Add serialize/deserialize API test coverage. (`#118 <https://github.com/ros2/rmw_implementation/issues/118>`_)
* Add subscription API test coverage. (`#117 <https://github.com/ros2/rmw_implementation/issues/117>`_)
* Extend publisher API test coverage (`#115 <https://github.com/ros2/rmw_implementation/issues/115>`_)
* Add node construction/destruction API test coverage. (`#112 <https://github.com/ros2/rmw_implementation/issues/112>`_)
* Check that rmw_init() fails if no enclave is given. (`#113 <https://github.com/ros2/rmw_implementation/issues/113>`_)
* Contributors: Alejandro Hernández Cordero, Chris Lalancette, Geoffrey Biggs, Jose Tomas Lorente, José Luis Bueno López, Michel Hidalgo

2.0.0 (2020-07-08)
------------------
* Add init options API test coverage. (`#108 <https://github.com/ros2/rmw_implementation/issues/108>`_)
* Complete init/shutdown API test coverage. (`#107 <https://github.com/ros2/rmw_implementation/issues/107>`_)
* Add dependency on ament_cmake_gtest (`#109 <https://github.com/ros2/rmw_implementation/issues/109>`_)
* Add test_rmw_implementation package. (`#106 <https://github.com/ros2/rmw_implementation/issues/106>`_)
* Contributors: Ivan Santiago Paunovic, Michel Hidalgo, Shane Loretz
