// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "../src/webhead.cc"
#include <stdio.h>
#include <stdint.h>

using namespace WebHead;

int
main (int argc, const char *argv[])
{
  const std::vector<WebHeadBrowser>  browsers = web_head_find (WebHeadType::Chromium);
  for (const WebHeadBrowser &b : browsers)
    printf ("Browser: type=%d%s\t%-32s version %-16s\t\t(%s)\n", int (b.type), b.snapdir ? " (snap):" : ":    ", b.executable.c_str(), b.version.c_str(), b.identification.c_str());
  if (browsers.size()) {
    static WebHeadSession wh ("https://github.com/tim-janik/webhead/blob/trunk/examples/hello.cc", __FILE__);
    printf ("%s:%s: starting web head: %s\n", __FILE__, __func__, browsers[0].executable.c_str());
    int err = wh.start (browsers[0]);
    printf ("%s:%s: %s: running=%d: %s\n", __FILE__, __func__, browsers[0].executable.c_str(), wh.running(), strerror (err));
    while (wh.running())
      sleep (1);
  };
  return 0;
}
