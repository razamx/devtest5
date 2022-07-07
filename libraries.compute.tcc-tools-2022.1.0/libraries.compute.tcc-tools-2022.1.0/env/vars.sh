#!/bin/sh
# shellcheck shell=sh

# Copyright Intel Corporation
# SPDX-License-Identifier: MIT
# https://opensource.org/licenses/MIT

# ############################################################################

# Get absolute path to script, when sourced from bash, zsh and ksh shells.
# Uses `readlink` to remove links and `pwd -P` to turn into an absolute path.

# Usage:
#   script_dir=$(get_script_path "$script_rel_path")
#
# Inputs:
#   script/relative/pathname/scriptname
#
# Outputs:
#   /script/absolute/pathname

# executing function in a *subshell* to localize vars and effects on `cd`
get_script_path() (
  script="$1"
  while [ -L "$script" ] ; do
    # combining next two lines fails in zsh shell
    script_dir=$(command dirname -- "$script")
    script_dir=$(cd "$script_dir" && command pwd -P)
    script="$(readlink "$script")"
    case $script in
      (/*) ;;
       (*) script="$script_dir/$script" ;;
    esac
  done
  # combining next two lines fails in zsh shell
  script_dir=$(command dirname -- "$script")
  script_dir=$(cd "$script_dir" && command pwd -P)
  echo "$script_dir"
)

# ############################################################################

# Prevent double source of script

if [ "${TCC_ENV_COMPLETED}" = "1" ]; then
  echo ":: WARNING: script has already been run. Skipping re-execution."
  return 3
fi

# ############################################################################

# This script is designed to be POSIX compatible, there are a few lines in the
# code block below that contain elements that are specific to a shell. The
# shell-specific elements are needed to identify the sourcing shell.

usage() {
  printf "%s\n"   "ERROR: This script must be sourced."
  printf "%s\n"   "Usage: source $1"
  exit 255
}

vars_script_name=""

if [ -n "${ZSH_VERSION:-}" ] ; then     # only executed in zsh
  case $ZSH_EVAL_CONTEXT in (*:file*) vars_script_name="${(%):-%x}" ;; esac ;
  if [ -n "$ZSH_EVAL_CONTEXT" ] && [ "" = "$vars_script_name" ] ; then
    usage "${(%):-%x}" ;
  fi
elif [ -n "${KSH_VERSION:-}" ] ; then   # only executed in ksh or mksh or lksh
  if [ "$(set | grep KSH_VERSION)" = "KSH_VERSION=.sh.version" ] ; then # ksh
    if [ "$(cd "$(dirname -- "$0")" && pwd -P)/$(basename -- "$0")" \
      != "$(cd "$(dirname -- "${.sh.file}")" && pwd -P)/$(basename -- "${.sh.file}")" ] ; then
      vars_script_name="${.sh.file}" || usage "$0" ;
    fi
  else # mksh or lksh detected (also check for [lm]ksh renamed as ksh)
    _lmksh="$(basename -- "$0")" ;
    if [ "mksh" = "$_lmksh" ] || [ "lksh" = "$_lmksh" ] || [ "ksh" = "$_lmksh" ] ; then
      # force [lm]ksh to issue error msg; contains this script's rel/path/filename
      vars_script_name="$( (echo "${.sh.file}") 2>&1 )" || : ;
      vars_script_name="$(expr "$vars_script_name" : '^.*ksh: \(.*\)\[[0-9]*\]:')" ;
    fi
  fi
elif [ -n "${BASH_VERSION:-}" ] ; then  # only executed in bash
  # shellcheck disable=2128,2015
  (return 0 2>/dev/null) && vars_script_name="${BASH_SOURCE}" || usage "${BASH_SOURCE}"
else
  case ${0##*/} in (sh|dash) vars_script_name="" ;; esac
fi

if [ "" = "$vars_script_name" ] ; then
  >&2 echo ":: ERROR: Unable to proceed: no support for sourcing from '[dash|sh]' shell." ;
  >&2 echo "   This script must be sourced. Did you execute or source this script?" ;
  >&2 echo "   Can be caused by sourcing from inside a \"shebang-less\" script." ;
  >&2 echo "   Can also be caused by sourcing from ZSH version 4.x or older." ;
  return 1 2>/dev/null || exit 1
fi

# ############################################################################

# Set required environment variables

script_path=$(get_script_path "${vars_script_name:-}")

I_TCC_ROOT=$(dirname ${script_path})

CPATH=${I_TCC_ROOT}/include${CPATH+:${CPATH}}
LIBRARY_PATH=${I_TCC_ROOT}/lib64${LIBRARY_PATH+:${LIBRARY_PATH}}
LD_LIBRARY_PATH=${I_TCC_ROOT}/lib64${LD_LIBRARY_PATH+:${LD_LIBRARY_PATH}}
PATH=${I_TCC_ROOT}/bin${PATH+:${PATH}}

TCC_ROOT=${I_TCC_ROOT}
TCC_CONFIG_PATH=${I_TCC_ROOT}/config
TCC_TOOLS_PATH=${I_TCC_ROOT}/tools
TCC_ENV_COMPLETED=1

export PATH
export CPATH
export LIBRARY_PATH
export LD_LIBRARY_PATH

export TCC_ROOT
export TCC_CONFIG_PATH
export TCC_TOOLS_PATH
export TCC_ENV_COMPLETED
