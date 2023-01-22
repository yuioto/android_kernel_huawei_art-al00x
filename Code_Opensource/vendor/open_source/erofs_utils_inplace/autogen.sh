#!/bin/bash
# build erofs-utils
# Copyright © SPDX-License-Identifier: GPL-2.0+
set -e
aclocal && \
autoheader && \
autoconf && \
libtoolize && \
automake -a -c

