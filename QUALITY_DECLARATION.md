This document is a declaration of software quality for the `rmw_implementation` package, based on the guidelines in [REP-2004](https://www.ros.org/reps/rep-2004.html).

# `rmw_implementation` Quality Declaration

The package `rmw_implementation` claims to be in the **Quality Level 1** category.

Below are the rationales, notes, and caveats for this claim, organized by each requirement listed in the [Package Requirements for Quality Level 1 in REP-2004](https://www.ros.org/reps/rep-2004.html).

## Version Policy [1]

### Version Scheme [1.i]

`rmw_implementation` uses `semver` according to the recommendation for ROS Core packages in the [ROS 2 Developer Guide](https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#versioning), and is at or above a stable version, i.e. `>= 1.0.0`.

**TODO** The current version is 0.8.2.

### Version Stability [1.ii]

### Public API Declaration [1.iii]

**`rmw_implementation` doesn't expose a public API.**

### API Stability Within a Released ROS Distribution [1.iv]/[1.vi]

`rmw_implementation` will not break public API within a released ROS distribution, i.e. no major releases once the ROS distribution is released.

### ABI Stability Within a Released ROS Distribution [1.v]/[1.vi]

`rmw_implementation` will maintain ABI stability within a ROS distribution.

## Change Control Process [2]

`rmw_implementation` follows the recommended guidelines for ROS Core packages in the [ROS 2 Developer Guide](https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#change-control-process).

### Change Requests [2.i]

All changes occur through a pull request

### Contributor Origin [2.ii]

Must have confirmation of contributor origin. DCO is activated

### Peer Review Policy [2.iii]

All pull request have two peer reviews

### Continuous Integration [2.iv]

All pull request must pass CI on all [tier 1 platforms](https://www.ros.org/reps/rep-2000.html#support-tiers)

### Documentation Policy [2.v]

All pull request must resolve related documentation changes before merging

## Documentation [3]

### Feature Documentation [3.i]

`rmw_implementation` has a [feature list](TODO) and each item in the list links to the corresponding feature documentation.
There is documentation for all of the features, and new features require documentation before being added.

### Public API Documentation [3.ii]

`rmw_implementation` doesn't expose a public API.

### License [3.iii]

The license for `rmw_implementation` is Apache 2.0, and a summary is in each source file, the type is declared in the `package.xml` manifest file, and a full copy of the license is in the [LICENSE](./LICENSE) file.

There is an automated test which runs a linter (ament_copyright) that ensures each file has a license statement.

### Copyright Statement [3.iv]

The copyright holders each provide a statement of copyright in each source code file in `rmw_implementation`.

There is an automated test which runs a linter (ament_copyright) that ensures each file has at least one copyright statement.

## Testing [4]

### Feature Testing [4.i]

`rmw_implementation` uses and passes all the standard linters and static analysis tools for a C++ package as described in the [ROS 2 Developer Guide](https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#linters-and-static-analysis).

### Public API Testing [4.ii]

`rmw_implementation` doesn't expose a public API.

### Coverage [4.iii]

`rmw_implementation` follows the recommendations for ROS Core packages in the [ROS 2 Developer Guide](https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#coverage), and opts to use branch coverage instead of line coverage.

This includes:

- tracking and reporting branch coverage statistics
- achieving and maintaining branch coverage at or above 95%
- no lines are manually skipped in coverage calculations

Changes are required to make a best effort to keep or increase coverage before being accepted, but decreases are allowed if properly justified and accepted by reviewers.

Current coverage statistics can be viewed here:

TODO FIXME

![](https://codecov.io/gh/ahcorde/rmw_implementation/branch/ahcorde_code_coverage/graphs/sunburst.svg)

### Performance [4.iv]

### Linters and Static Analysis [4.v]

`rmw_implementation` uses and passes all the standard linters and static analysis tools for a C++ package as described in the [ROS 2 Developer Guide](https://index.ros.org/doc/ros2/Contributing/Developer-Guide/#linters-and-static-analysis).

TODO any qualifications on what "passing" means for certain linters

## Dependencies [5]

### Direct Runtime ROS Dependencies [5.i]

### Optional Direct Runtime ROS Dependencies [5.ii]

### Direct Runtime non-ROS Dependency [5.iii]

`rmw_implementation` has run-time and build-time dependencies that need to be considered for this declaration.

 - rcpputils
 - rcutils
 - rmw
 - rmw_implementation_cmake

It has one "buildtool" dependency, which do not affect the resulting quality of the package, because they do not contribute to the public library API.
It also has several test dependencies, which do not affect the resulting quality of the package, because they are only used to build and run the test code.

## Platform Support [6]

`rmw_implementation` supports all of the tier 1 platforms as described in [REP-2000](https://www.ros.org/reps/rep-2000.html#support-tiers), and tests each change against all of them.
