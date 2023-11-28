# Changelog

This changelog does not follow semantic versioning.


## UNRELEASED

### Added

- Added a little spinner to indicate network activity
- Added Windows 10 MSYS2 to list of confirmed-to-work platforms
- Added a new action `set-visibility` to the repos subcommand that
  allows updating the visibility level of a repository.
- Add a new action `request-review` to the pulls subcommand that
  allows requesting a review of a pull request from a given user.
- One can now define custom aliases in the alias section of the
  config file. Aliases are very primitive as of now. This means they
  are just different names for subcommands. Aliases may reference
  other aliases.
- Add a new `-M` flag to both the pulls and the issues subcommand
  to allow filtering by milestones.
- Add a new `patch` action to the pulls subcommand. This allows you
  to print the entire patch series for the given pull request.
- Add a new `title` action to both the issues and the pulls
  subcommand that allows updating their titles.

### Fixed

- Fixed incorrect internal help message of the `repos` subcommand.
- Worked around ICE with xlC 16 on ppc64le Debian Linux, gcli now
  compiles using xlC and works too.
- Fixed various memory leaks. This was not a real issue as these
  leaks were minor and gcli is not a long-running application where
  thease leaks would have had any serious impact on the memory
  footprint.
- Spelling fixes in manual pages (submitted by Jakub Wilk
  https://github.com/herrhotzenplotz/gcli/pull/121)

### Changed

- Subcommands can now be abbreviated by providing an unambiguous
  prefix that matches the subcommand.

### Removed


## 2.0.0 (2023-Sep-21)

### Added

- This changelog has been added
- gcli is now built as a shared or static library which the gcli tool links against
  This implied so many changes that the major version number was bumped.
- Added a package-config file for libgcli
- Added a `-L` flag to the `issues` and `pulls` subcommand to allow
  filtering by label
- A work-in-progress tutorial has been added and is available at
  [the GCLI directory](https://herrhotzenplotz.de/gcli/tutorial) on
  my website.
- Gitlab jobs now show coverage information

### Fixed

- Parallel builds in autotools have been re-enabled
- Improved error messages in various places
- Bad roff syntax in manual pages has been fixed

### Changed

- the `gcli pulls create` subcommand does not print the URL to the
  created release anymore.
- The test suite is now using [atf-c](https://github.com/jmmv/atf)
  and [kyua](https://github.com/jmmv/kyua). These are dependencies
  if you want to run the tests. These tools are installed out of
  the box on most BSDs.
- A newly introduced dependency is the `sys/queue.h` header. On
  GNU/Linux systems you might need to install it as part of libbsd.

### Removed

- The reviews subcommand has been removed because it was generally useless
  This feature will be reimplemented as a WIP of
  [#189](https://gitlab.com/herrhotzenplotz/gcli/-/issues/189)
