// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "../src/webhead.cc"
#include <stdio.h>
#include <stdint.h>

using namespace WebHead;

int
main (int argc, const char *argv[])
{
  // Find a suitable web head
  const std::vector<BrowserInfo>  browsers = web_head_find ();
  if (browsers.empty()) {
    dprintf (2, "%s:%s: Failed to find any usable web browser in $PATH\n", __FILE__, __func__);
    return 2;
  }
  for (const BrowserInfo &b : browsers)
    printf ("%s:%s: Found browser type=%d%s %-30s version %-14s\t\t(%s)\n", __FILE__, __func__, int (b.type), b.snapdir ? "snap:" : ":    ", b.executable.c_str(), b.version.c_str(), b.identification.c_str());
  // Run UI, simply picking the first suitable browser
  if (browsers.size()) {
    static Session wh ("https://github.com/tim-janik/webhead/blob/trunk/examples/hello.cc", __FILE__);
    printf ("%s:%s: starting web head: %s\n", __FILE__, __func__, browsers[0].executable.c_str());
    int err = wh.start (browsers[0]);
    printf ("%s:%s: %s: running=%d: %s\n", __FILE__, __func__, browsers[0].executable.c_str(), wh.running(), strerror (err));
    // Kill web head after some time
    // sleep (5); wh.kill();
    // Or keep running as long as the web head is running
    while (wh.running())
      sleep (1);
  };
  return 0;
}
