#!/usr/bin/env bash
# This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
set -Eeuo pipefail

SCRIPTNAME=${0##*/} ; die() { [ -z "$*" ] || echo "$SCRIPTNAME: $*" >&2; exit 9 ; }

# == Version from `git archive` ==
# Git archive replaces '$Format$' strings, according to gitattributes(5)
HASH='$Format:%H$'
VDATE='$Format:%ci$'
DESCRIBE='$Format:%(describe:match=v[0-9]*.[0-9]*.[0-9]*)$'

# == Version from git ==
if ! [[ "$HASH" =~ ^[0-9a-f]+$ ]] ; then		# If hash is not filled in
  DESCRIBE=
  if git rev-parse --git-dir >/dev/null 2>&1 ; then	# Fetch version from git
    HASH=$(git log -1 --pretty="tformat:%H")
    VDATE=$(git log -1 --pretty="tformat:%ci")
    # Match lightweight tags directly (for nightly, etc), or use annotated release tags
    DESCRIBE=$(git describe --tags --match='v[0-9]*.[0-9]*.[0-9]*' --exact-match 2>/dev/null ||
		 git describe --match='v[0-9]*.[0-9]*.[0-9]*')
  fi
fi

# == Fallback version ==
if test -z "$DESCRIBE" ; then
  HASH=0000000000000000000000000000000000000000
  DESCRIBE=v0.0.0-unversioned0
  VDATE="2001-01-01 01:01:01 +0000"
fi

# == Produce: VERSION HASH DATE ==
VERSIONNUMBER=$(echo "${DESCRIBE#v}" |
		  sed -e 's/-g[0-9a-f]\+$//i' \
		      -e 's/-\([0-9]\+\)$/.dev\1/')	# strip ^v, -gCOMMIT, enforce .devXX
echo "$VERSIONNUMBER" "$HASH" "$VDATE"
