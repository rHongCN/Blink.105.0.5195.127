#!/usr/bin/env python
# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Code generator for PlatformObject<> constructor list.

This script takes as arguments a list of platform names as a text file and
a list of types and generates a C++ source file containing a list of
the constructors for that object in platform order.

Example Output: ./ui/ozone/generate_constructor_list.py \
                    --platform test \
                    --platform dri \
                    --export OZONE \
                    --namespace ui \
                    --typename OzonePlatform \
                    --include '"ui/ozone/ozone_platform.h"'

  // DO NOT MODIFY. GENERATED BY generate_constructor_list.py

  #include "ui/ozone/platform_object_internal.h"

  #include "ui/ozone/ozone_platform.h"

  namespace ui {

  OzonePlatform* CreateOzonePlatformTest();
  OzonePlatform* CreateOzonePlatformDri();

  }  // namespace ui

  namespace ui {

  typedef ui::OzonePlatform* (*OzonePlatformConstructor)();

  template <> const OzonePlatformConstructor
  PlatformConstructorList<ui::OzonePlatform>::kConstructors[] = {
    &ui::CreateOzonePlatformTest,
    &ui::CreateOzonePlatformDri,
  };

  template class COMPONENT_EXPORT(OZONE) PlatformObject<ui::OzonePlatform>;

  }  // namespace ui
"""

try:
    from StringIO import StringIO  # for Python 2
except ImportError:
    from io import StringIO  # for Python 3
import optparse
import os
import collections
import re
import sys


def GetTypedefName(typename):
  """Determine typedef name of constructor for typename.

  This is just typename + "Constructor".
  """

  return typename + 'Constructor'


def GetConstructorName(typename, platform):
  """Determine name of static constructor function from platform name.

  This is just "Create" + typename + platform.
  """

  return 'Create' + typename + platform.capitalize()


def GenerateConstructorList(out, namespace, export, typenames, platforms,
                            includes, usings):
  """Generate static array containing a list of constructors."""

  out.write('// DO NOT MODIFY. GENERATED BY generate_constructor_list.py\n')
  out.write('\n')

  out.write('#include "ui/ozone/platform_object_internal.h"\n')
  out.write('\n')

  for include in includes:
    out.write('#include %(include)s\n' % {'include': include})
  out.write('\n')

  for using in usings:
    out.write('using %(using)s;\n' % {'using': using})
  out.write('\n')

  out.write('namespace %(namespace)s {\n' % {'namespace': namespace})
  out.write('\n')

  # Declarations of constructor functions.
  for typename in typenames:
    for platform in platforms:
      constructor = GetConstructorName(typename, platform)
      out.write('%(typename)s* %(constructor)s();\n'
               % {'typename': typename,
                  'constructor': constructor})
    out.write('\n')

  out.write('}  // namespace %(namespace)s\n' % {'namespace': namespace})
  out.write('\n')

  out.write('namespace ui {\n')
  out.write('\n')

  # Handy typedefs for constructor types.
  for typename in typenames:
    out.write('typedef %(typename)s* (*%(typedef)s)();\n'
              % {'typename': typename,
                 'typedef': GetTypedefName(typename)})
  out.write('\n')

  # The actual constructor lists.
  for typename in typenames:
    out.write('template <> const %(typedef)s\n'
              % {'typedef': GetTypedefName(typename)})
    out.write('PlatformConstructorList<%(typename)s>::kConstructors[] = {\n'
              % {'typename': typename})
    for platform in platforms:
      constructor = GetConstructorName(typename, platform)
      out.write('  &%(namespace)s::%(constructor)s,\n'
                % {'namespace': namespace, 'constructor': constructor})
    out.write('};\n')
    out.write('\n')

  # Exported template instantiation.
  for typename in typenames:
    out.write('template class COMPONENT_EXPORT(%(export)s)' \
              ' PlatformObject<%(typename)s>;\n'
              % {'export': export, 'typename': typename})
  out.write('\n')

  out.write('}  // namespace ui\n')
  out.write('\n')


def main(argv):
  parser = optparse.OptionParser()
  parser.add_option('--namespace', default='ozone')
  parser.add_option('--export', default='OZONE')
  parser.add_option('--platform_list')
  parser.add_option('--output_cc')
  parser.add_option('--include', action='append', default=[])
  parser.add_option('--platform', action='append', default=[])
  parser.add_option('--typename', action='append', default=[])
  parser.add_option('--using', action='append', default=[])
  options, _ = parser.parse_args(argv)

  platforms = list(options.platform)
  typenames = list(options.typename)
  includes = list(options.include)
  usings = list(options.using)

  if options.platform_list:
    platforms = open(options.platform_list, 'r').read().strip().split('\n')

  if not platforms:
    sys.stderr.write('No platforms are selected!')
    sys.exit(1)

  # Write to standard output or file specified by --output_cc.
  out_cc = getattr(sys.stdout, 'buffer', sys.stdout)
  if options.output_cc:
    out_cc = open(options.output_cc, 'wb')

  out_cc_str = StringIO()
  GenerateConstructorList(out_cc_str, options.namespace, options.export,
                          typenames, platforms, includes, usings)
  out_cc.write(out_cc_str.getvalue().encode('utf-8'))

  if options.output_cc:
    out_cc.close()

  return 0


if __name__ == '__main__':
  sys.exit(main(sys.argv[1:]))
