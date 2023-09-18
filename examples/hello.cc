// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "../src/webhead.cc"
#include <stdio.h>
#include <stdint.h>

using namespace WebHead;

int
main (int argc, const char *argv[])
{
  const std::vector<WebHeadBrowser>  browsers = web_head_find();
  for (const WebHeadBrowser &b : browsers)
    printf ("Browser: type=%d%s\t%-32s version %-16s\t\t(%s)\n", int (b.type), b.snapdir ? " (snap):" : ":    ", b.executable.c_str(), b.version.c_str(), b.identification.c_str());

  return 0;
}
