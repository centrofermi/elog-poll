#!/bin/bash
#
# elog-poll
#
# This script repeatedly checks whether new elog posts have been added
# to a specific ELOG logbook and eventually executes a program to handle
# the post.
#
# Author: Carmelo Pellegrino <carmelo.pellegrino@cnaf.infn.it>
#

# The configuration of this very script is perfomed via shell variables.
# Here the list of parameters is reported:
#
# - ELOG_EXE       -> Path to the 'elog' executable
# - ELOG_SERVER    -> Address of the ELOG server
# - ELOG_PORT      -> Port of the ELOG server
# - ELOG_USES_SSL  -> Specify the use of SSL for connecting to ELOG [y|n]
# - ELOG_BOOK      -> Logbook to poll
# - ELOG_USER      -> User name for reading ELOG posts
# - ELOG_PASSWORD  -> Password of the user specified in the variable above
# - ELOG_LAST_POST -> Path of the file containing the id of the first post
#                     to check
# - ELOG_PARSER    -> Path to the elog-parse executable
# - ELOG_PRODUCER  -> Path to the data producer executable
#

# The user may want to set the configuration variables here
#ELOG_EXE=
#ELOG_SERVER=
#ELOG_PORT=
#ELOG_USES_SSL=
#ELOG_BOOK=
#ELOG_USER=
#ELOG_PASSWORD=
#ELOG_LAST_POST=
#ELOG_PARSER=
#ELOG_PRODUCER=

readonly REPLY_EC=0
readonly INVALID_EC=1

function show_help() {
  echo "Usage:"
  echo "  $(basename "$0") [id]"
  echo "if the id parameter is specified, the program checks that ID only,"
  echo "otherwise it checks all the IDs starting from that stored in the"
  echo "${ELOG_LAST_POST} file."
}

function check_config() {
  local missing=()
  [ -n "${ELOG_EXE+x}"       ] || missing+=(ELOG_EXE)
  [ -n "${ELOG_SERVER+x}"    ] || missing+=(ELOG_SERVER)
  [ -n "${ELOG_PORT+x}"      ] || missing+=(ELOG_PORT)
  [ -n "${ELOG_USES_SSL+x}"  ] || missing+=(ELOG_USES_SSL)
  [ -n "${ELOG_BOOK+x}"      ] || missing+=(ELOG_BOOK)
  [ -n "${ELOG_USER+x}"      ] || missing+=(ELOG_USER)
  [ -n "${ELOG_PASSWORD+x}"  ] || missing+=(ELOG_PASSWORD)
  [ -n "${ELOG_LAST_POST+x}" ] || missing+=(ELOG_LAST_POST)
  [ -n "${ELOG_PARSER+x}"    ] || missing+=(ELOG_PARSER)
  [ -n "${ELOG_PRODUCER+x}"  ] || missing+=(ELOG_PRODUCER)

  if [ ! ${#missing} -eq 0 ]; then
    echo "ERROR: the following environment variables are missing:" >&2
    for miss in ${missing[*]}; do
      echo "${miss}" >&2
    done
    echo "Please set them before running again this program." >&2

    return 1
  fi

  return 0
}

function on_exit() {
  set_last_post_id "${ID}"
  echo "Bye!"
}

function get_last_post_id() {
  [ ! -e "${ELOG_LAST_POST}" ] && echo 1 > "${ELOG_LAST_POST}"
  cat "${ELOG_LAST_POST}"
}

function set_last_post_id() {
  echo "$1" >"${ELOG_LAST_POST}"
}

function get_post() {
  local ID=$1
  local ssl=""
  [ "${ELOG_USES_SSL}" == "y" ] && ssl="-s"

  ${ELOG_EXE}                             \
    -w "${ID}" -l "${ELOG_BOOK}" "${ssl}" \
    -h "${ELOG_SERVER}" -p "${ELOG_PORT}" \
    -u "${ELOG_USER}" "${ELOG_PASSWORD}"
}

function reply_post() {
  local ID=$1
  local message="$2"
  shift 2

  local attachments=()
  for item in "$@"; do
    attachments+=(-f "${item}")
  done

  local ssl=""
  [ "${ELOG_USES_SSL}" == "y" ] && ssl="-s"

  ${ELOG_EXE}                             \
    -r "${ID}" -l "${ELOG_BOOK}" "${ssl}" \
    -h "${ELOG_SERVER}" -p "${ELOG_PORT}" \
    -u "${ELOG_USER}" "${ELOG_PASSWORD}"  \
    "${attachments[@]}" "${message}"
}

function epoch2date() {
  date +%Y-%m-%d -d @"$1"
}

function handler() {
  local -r data="$(cat)"

  echo "${data}" | "${ELOG_PARSER}" -c || return ${INVALID_EC}
  echo "${data}" | "${ELOG_PARSER}" -r && return ${REPLY_EC}

  local -r id="$(echo "${data}" | "${ELOG_PARSER}" '%[id]')"
  local -r start_time="$(echo "${data}" | "${ELOG_PARSER}" '%[Start time]')"
  local -r stop_time="$(echo "${data}" | "${ELOG_PARSER}" '%[Stop time]')"

  local -r start_date="$(epoch2date "${start_time}")"
  local -r stop_date="$(epoch2date "${stop_time}")"

  local -r cut="$(echo "${data}" | "${ELOG_PARSER}" '%[Cut]')"
  local -r format="$(echo "${data}" | "${ELOG_PARSER}" '%[Output format]')"
  local -r telescope_id="$(echo "${data}" | "${ELOG_PARSER}" '%[Telescope ID]')"

  local -r param_types=(I I I F F F F F F)
  local -r parameters=(RunNumber Seconds Nanoseconds Theta Phi ChiSquare \
                       TimeOfFlight TrackLength DeltaTime)

  local options=()
  for ((i = 0; i < "${#parameters[@]}"; ++i)); do
    local parameter="${parameters[$i]}"
    local type="${param_types[$i]}"

    if [ "$(echo "${data}" | "${ELOG_PARSER}" "%[${parameter}]")" == "1" ]; then
      options+=("${type}" "${parameter}")
    fi
  done

  local answer
  answer="$("${ELOG_PRODUCER}" "${format}" "${telescope_id}" \
                     "${start_date}" "${stop_date}" "${cut}" \
                     "${options[@]}")"
  local -r return_status=$?

  if [ "${return_status}" -eq 0 ]; then
    zip "${answer}.zip" "${answer}"

    reply_post "${id}" "Data extraction succeeded" "${answer}.zip"
  else
    reply_post "${id}" "Data extraction failed: ${answer}"
  fi
}

# Script begins here
if [ "$1" == "-h" ] || [ "$1" == "--help" ]; then
  show_help
  exit 0
fi

check_config || exit 1

if [ -n "$1" ]; then
  # Handle a specific ELOG ID
  get_post "$1" | handler
else
  trap on_exit EXIT
  # Handle the whole archive from the last checked
  ID=$(get_last_post_id)
  while true; do
    get_post "${ID}" | handler || break
    ((++ID))
  done
fi

