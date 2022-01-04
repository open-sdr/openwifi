#!/bin/sh

# Author: Xianjun Jiao
# SPDX-FileCopyrightText: 2019 UGent
# SPDX-License-Identifier: AGPL-3.0-or-later

VERSION="3.17"
OUT="$1"

if [ -d .git ] && head=`git rev-parse --verify HEAD 2>/dev/null`; then
	git update-index --refresh --unmerged > /dev/null
	descr=$(git describe)

	# on git builds check that the version number above
	# is correct...
	[ "${descr%%-*}" = "v$VERSION" ] || exit 2

	v="${descr#v}"
	if git diff-index --name-only HEAD | read dummy ; then
		v="$v"-dirty
	fi
else
	v="$VERSION"
fi

echo '#include "sdrctl.h"' > "$OUT"
echo "const char sdrctl_version[] = \"$v\";" >> "$OUT"
