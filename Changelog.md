# Changelog

This changelog does not follow semantic versioning.

## Version 2.0.0 (2023-Sep-20)

### Added

- This changelog
- gcli is now built as a shared or static library which the gcli tool links against
- Added a package-config file for libgcli
- Added a `-L` flag to the `issues` subcommand to allow filtering by author

### Fixed

- Parallel builds in autotools have been re-enabled

### Removed

- The reviews subcommand has been removed because it was generally useless
  This feature will be reimplemented as a WIP of [#189](https://gitlab.com/herrhotzenplotz/gcli/-/issues/189)
