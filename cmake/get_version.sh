#!/bin/sh
# Copyright (C) 2022-2023 Adam Lugowski. All rights reserved.
# Use of this source code is governed by the BSD 2-clause license found in the LICENSE.txt file.

# Extract the version number from #defines in a header file. Looking for defines that end in:
# _VERSION_MAJOR
# _VERSION_MINOR
# _VERSION_PATCH
# each followed by an integer.

get_version_part()
{
	grep "^#define\(.*\)$2" $1 | sed 's/[^0-9]*//g'
}

VERSION_FILE="$1"

VERSION_MAJOR=$(get_version_part "${VERSION_FILE}" "_VERSION_MAJOR")
VERSION_MINOR=$(get_version_part "${VERSION_FILE}" "_VERSION_MINOR")
VERSION_PATCH=$(get_version_part "${VERSION_FILE}" "_VERSION_PATCH")

#echo "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
printf "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"