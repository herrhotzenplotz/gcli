# Changelog

This changelog does not follow semantic versioning.


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
