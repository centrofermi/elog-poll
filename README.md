# ELOG-poll
[![Check status](https://travis-ci.org/centrofermi/elog-poll.svg?branch=master)](https://travis-ci.org/centrofermi/elog-poll)

## Description
ELOG-poll is a shell script (BASH) designed to simplify and automatise
interfacing with an [ELOG](https://midas.psi.ch/elog/) server.
It runs in a Linux box with basic command line tools installed.
It might work also on macOS and Windows Subsystem for Linux.
In particular, it wraps around the `elog` command line program which is
shipped together with the ELOG source code.

## Configuration
The program is configured via a set of environment variables and it checks
whether they are set during start-up and will notify the user about
missing ones.

Here the list of the environment variables is shown:
* ELOG\_EXE:        Path to the `elog` executable
* ELOG\_SERVER:     Address of the ELOG server
* ELOG\_PORT:       Port of the ELOG server
* ELOG\_USES\_SSL:  Specify the use of SSL for connecting to ELOG [y|n]
* ELOG\_BOOK:       Logbook to poll
* ELOG\_USER:       User name for reading ELOG posts
* ELOG\_PASSWORD:   Password of the user specified in the variable above
* ELOG\_LAST\_POST: Path of the file containing the id of the first post to check
* ELOG\_PARSER:     Path to the elog-parse executable
* ELOG\_PRODUCER:   Path to the data producer executable

These can be set externally via command line - either manually or in a
shell file automatically loaded, e.g. `.bashrc` - or written inside the
script itself. For the latter solution, there is some room already
allocated on top of the file.

**NOTE**: when setting from external environment, make sure the variables
are `export`ed.

**NOTE**: the values set inside the script override the values set
externally.

## Usage
Run the `elog-poll.sh` script from the command line.

Type
```shell
./elog-poll.sh -h # or --help
```
to get the help.

## Dependencies
* GNU bash
* GNU coreutils

