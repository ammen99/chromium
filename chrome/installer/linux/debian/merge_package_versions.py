#!/usr/bin/env python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import sys

import package_version_interval

if len(sys.argv) < 3:
  print ('Usage: %s output_deps_file input1_deps_file input2_deps_file ...' %
         sys.argv[0])
  sys.exit(1)

output_filename = sys.argv[1]
input_filenames = sys.argv[2:]

package_intervals = {}
for input_filename in input_filenames:
  for line in open(input_filename):
    # Allow comments starting with '#'
    if line.startswith('#'):
      continue
    line = line.rstrip('\n')
    (package, interval) = package_version_interval.parse_dep(line)
    if package in package_intervals:
      interval = interval.intersect(package_intervals[package])
    package_intervals[package] = interval

with open(output_filename, 'w') as output_file:
  output_file.write(package_version_interval.format_package_intervals(
      package_intervals))
