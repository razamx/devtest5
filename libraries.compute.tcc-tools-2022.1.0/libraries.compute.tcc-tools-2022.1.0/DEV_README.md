# Welcome to Intel® Time Coordinated Computing (Intel® TCC)

## Project Repositories ##

The project contains the following repositories:

- **tccsdk** - repository for Intel(R) TCC tools sources.
- **tccdev** - repository for Intel(R) TCC tools scans/validations etc.
- **tcc-docs** - repository for Intel(R) TCC IDZ documentation.

### Branching Strategy ###

The `master` branch is the main branch where development
happens. Feature, release and hotfix branches have limited lifetimes.

#### Feature Branches ####

May branch off from:
`master`
Must merge back into:
`master`
Branch naming convention:
   `feature/username/feature_name`
   `fix/username/fix_name`

Long lived branches based on rally items should have rally id followed
by slash followed by short descriptive name:
`feature/US1234/super-cool-feature`

#### Release Branches

May branch off from:
`master`
Must merge back into:
`master`
Release codes are tagged with the following name convention:
`vYYYY.V1.V2` for example, `v2021.1.1`

#### Hotfix Branches

May branch off from:
`master`
Must merge back into:
`master`
Hotfix codes are tagged with the following name convention:
`vYYYY.V1.V2.V3` for example, `v2021.1.1.1`

See [A successful Git branching model](http://nvie.com/posts/a-successful-git-branching-model/) for details on the approach we are using.

## Style Guide

TBD.

## Build prerequisites

To build Intel(R) TCC tools from sources you will need to install the following tools:

- TBD
- TBD

### Build configuration

Go to the directory where you cloned the `tccsdk` repository.
Create directory near to `tccsdk` directory to store build configuration files for the TCC tools project `mkdir build`, go to the created directory `cd build` and create build configuration
   `cmake -DPACKAGE_TYPE=NDA -DBUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/home/root/tcc/tcc-tools/install ../tccsdk/`

The following parameters are available:

- `PACKAGE_TYPE` - type of package to be created
  - `NDA`
  - TBD
- `BUILD_TESTS` - to build tests or not
  - `ON`
  - `OFF`
- `CMAKE_BUILD_TYPE` - type of build of TCC tools
  - `RELEASE`
  - TBD
- `CMAKE_INSTALL_PREFIX`=/dir - directory when to place built artifacts

## Contributing into `tcc-docs` repository

Before making changes to IDZ documentation located at `tcc-docs` repository read the following
Documentation Guide:
`https://tcc.gitlab-pages.devtools.intel.com/tcc-docs/doc-guide/index.html`

## Samples BKM's

Before development of a new sample, or making change to the existing one read the following
document with BKM's on how to develop samples:
`https://tcc.gitlab-pages.devtools.intel.com/tcc-docs/doc-guide/files/tcc_doc_bkm_sample.html`

## Things To Do Before Pushing To Repository

1. Make sure code follows the conventions.
2. Insure default build works.

## Submitting Changes To Repository

1. Start a Git review by doing the following:
    TBD: `git push origin HEAD:refs/for/<branch>`
