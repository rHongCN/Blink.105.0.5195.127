# Copyright (c) 2016 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
Parse information about a PE file to summarize the on-disk and
in-memory sizes of the sections, in decimal MB instead of in hex. This
script will also automatically display diffs between two files if they
have the same name. This script relies on having VS 2015 installed and is used
to help investigate binary size regressions and improvements.

Section information printed by dumpbin looks like this:

SECTION HEADER #2
  .rdata name
  5CCD56 virtual size
 1CEF000 virtual address (11CEF000 to 122BBD55)
  5CCE00 size of raw data
 1CEE000 file pointer to raw data (01CEE000 to 022BADFF)
       0 file pointer to relocation table
       0 file pointer to line numbers
       0 number of relocations
       0 number of line numbers
40000040 flags
         Initialized Data
         Read Only

The reports generated by this script look like this:

> python tools\win\pe_summarize.py out\release\chrome.dll
Size of out\release\chrome.dll is 41.190912 MB
      name:   mem size  ,  disk size
     .text: 33.199959 MB
    .rdata:  6.170416 MB
     .data:  0.713864 MB,  0.270336 MB
      .tls:  0.000025 MB
  CPADinfo:  0.000036 MB
   .rodata:  0.003216 MB
  .crthunk:  0.000064 MB
    .gfids:  0.001052 MB
    _RDATA:  0.000288 MB
     .rsrc:  0.130808 MB
    .reloc:  1.410172 MB

Note that the .data section has separate in-memory and on-disk sizes due to
zero-initialized data. Other sections have smaller discrepancies - the disk size
is only printed if it differs from the memory size by more than 512 bytes.

Note that many of the sections - such as .text, .rdata, and .rsrc - are shared
between processes. Some sections - such as .reloc - are discarded after a
process is loaded. Other sections, such as .data, produce private pages and are
therefore objectively 'worse' than the others.
"""

from __future__ import print_function

import os
import subprocess
import sys


def _FindSection(section_list, section_name):
  for i in range(len(section_list)):
    if section_name == section_list[i][0]:
      return i
  return -1


def main():
  if len(sys.argv) < 2:
    print(r'Usage: %s PEFileName [OtherPeFileNames...]' % sys.argv[0])
    print(r'Sample: %s chrome.dll' % sys.argv[0])
    print(r'Sample: %s chrome.dll original\chrome.dll' % sys.argv[0])
    return 0

  # Track the name of the last PE (Portable Executable) file to be processed -
  # file name only, without the path.
  last_pe_filepart = ""

  for pe_path in sys.argv[1:]:
    results = []
    if not os.path.exists(pe_path):
      print('%s does not exist!' % pe_path)
      continue

    print('Size of %s is %1.6f MB' % (pe_path, os.path.getsize(pe_path) / 1e6))
    print('%10s:  %9s  ,  %9s' % ('name', 'mem size', 'disk size'))

    sections = None
    # Pass the undocumented /nopdb header to avoid hitting symbol servers for
    # the entrypoint name.
    command = 'dumpbin.exe /nopdb /headers "%s"' % pe_path
    try:
      for line in subprocess.check_output(command).decode().splitlines():
        if line.startswith('SECTION HEADER #'):
          sections = []
        elif type(sections) == type([]):
          # We must be processing a section header.
          sections.append(line.strip())
          # When we've accumulated four lines of data, process them.
          if len(sections) == 4:
            name, memory_size, _, disk_size = sections
            assert name.count('name') == 1
            assert memory_size.count('virtual size') == 1
            assert disk_size.count('size of raw data') == 1
            name = name.split()[0]
            memory_size = int(memory_size.split()[0], 16)
            disk_size = int(disk_size.split()[0], 16)
            # Print the sizes in decimal MB. This makes large numbers easier to
            # understand - 33.199959 is easier to read than 33199959. Decimal MB
            # is used to allow simple conversions to a precise number of bytes.
            if abs(memory_size - disk_size) < 512:
              print('%10s: %9.6f MB' % (name, memory_size / 1e6))
            else:
              print('%10s: %9.6f MB, %9.6f MB' % (name, memory_size / 1e6,
                                                  disk_size / 1e6))
            results.append((name, memory_size))
            sections = None
    except WindowsError as error:
      if error.winerror == 2:
        print (r'Cannot find dumpbin. Run "C:\Program Files (x86)\Microsoft '
               r'Visual Studio\2017\Professional\VC\Auxiliary\Build'
               r'\vcvarsall.bat amd64" or similar to add dumpbin to the path.')
      else:
        print(error)
      break

    print()
    pe_filepart = os.path.split(pe_path)[1]
    if pe_filepart.lower() == last_pe_filepart.lower():
      # Print out the section-by-section size changes, for memory sizes only.
      print('Memory size change from %s to %s' % (last_pe_path, pe_path))
      total_delta = 0
      for i in range(len(results)):
        section_name = results[i][0]
        # Find a matching section name. Mismatches can occur when comparing
        # 32-bit and 64-bit binaries. They can also occur when one of the
        # binaries pulls in code that defines custom sections such as .rodata.
        last_i = _FindSection(last_results, section_name)
        delta = results[i][1]
        if last_i >= 0:
          delta -= last_results[last_i][1]
        total_delta += delta
        if delta:
          print('%12s: %7d bytes change' % (section_name, delta))
      for last_i in range(len(last_results)):
        section_name = last_results[last_i][0]
        # Find sections that exist only in last_results.
        i = _FindSection(results, section_name)
        if i < 0:
          delta = -last_results[last_i][1]
          total_delta += delta
          print('%12s: %7d bytes change' % (section_name, delta))
      print('Total change: %7d bytes' % total_delta)
    last_pe_filepart = pe_filepart
    last_pe_path = pe_path
    last_results = results


if __name__ == '__main__':
  sys.exit(main())