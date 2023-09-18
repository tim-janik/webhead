// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "webhead.hh"
#include <stdlib.h>
#include <boost/process.hpp>
#include <filesystem>
#include <regex>

#define WEBHEAD_DEBUG(...)      do { if (0) dprintf (2, __VA_ARGS__); } while (0)

namespace WebHead {

/// Join a vector of strings.
static std::string
string_join (const std::string &junctor, const std::vector<std::string> &strvec)
{
  std::string s = strvec.size() ? strvec[0] : "";
  for (size_t i = 1; i < strvec.size(); i++)
    s += junctor + strvec[i];
  return s;
}

/// Run program and capture output.
static std::tuple<int,std::string,std::string>
synchronous_exec (const std::string &path, const std::vector<std::string> &args)
{
  namespace bp = boost::process;
  // bp::ipstream pout, perr;
  std::future<std::string> pout, perr;
  boost::asio::io_context ioc;
  bp::child cproc (path, bp::args (args), bp::std_out > pout, bp::std_err > perr, ioc);
  WEBHEAD_DEBUG ("%s: %s %s\n", __func__, path.c_str(), string_join (" ", args).c_str());
  ioc.run();
  cproc.wait();
  const int exit_code = cproc.exit_code();
  return { exit_code, pout.get(), perr.get() };
}

/// Gather capture groups from a regex search match
std::vector<std::string>
regex_capture (const std::string &regex, const std::string &input)
{
  std::regex rex (regex, std::regex::ECMAScript); // std::regex::icase
  std::smatch m;
  if (std::regex_search (input, m, rex)) {
    std::vector<std::string> subs;
    for (size_t i = 0; i < m.size(); i++)
      subs.push_back (m.str (i));
    return subs;
  }
  return {};
}

/// Check if a path exists
static bool
path_exists (const std::filesystem::path &path)
{
  std::error_code ec{};
  return std::filesystem::exists (path, ec) && !ec;
}

/// Path of the current users home directory
static std::string
home_dir()
{
  return getenv ("HOME");
}

// == detect existing browsers ==
struct BrowserCheck {
  std::string exename;
  std::string versionpattern;
  WebHeadType browsertype;
};
static const BrowserCheck web_head_browser_checks[] = {
  { "firefox",                  "(Mozilla\\s*)(Firefox\\s*)([0-9]+[-0-9.a-z+]*).*",       WebHeadType::Firefox },
  { "firefox-esr",              "(Mozilla\\s*)(Firefox\\s*)([0-9]+[-0-9.a-z+]*).*",       WebHeadType::Firefox },
  { "google-chrome",            "(Google\\s*)(Chrome\\s\\s*)([0-9]+[-0-9.a-z+]*).*",      WebHeadType::GoogleChrome },
  { "google-chrome-stable",     "(Google\\s*)(Chrome\\s\\s*)([0-9]+[-0-9.a-z+]*).*",      WebHeadType::GoogleChrome },
  { "chromium-browser",         "(Chromium\\s\\s*)([0-9]+[-0-9.a-z+]*).*",                WebHeadType::Chromium },
  { "chromium",                 "(Chromium\\s\\s*)([0-9]+[-0-9.a-z+]*).*",                WebHeadType::Chromium },
  { "epiphany-browser",         "(Web\\s\\s*)([0-9]+[-0-9.a-z+]*).*",                     WebHeadType::Epiphany },
};

/// Find and return a list of browsers in $PATH that can be used as web heads.
std::vector<WebHeadBrowser>
web_head_find (WebHeadType type)
{
  namespace bp = boost::process;
  namespace fs = std::filesystem;
  std::vector<WebHeadBrowser> browsers;
  for (size_t j = 0; j < sizeof (web_head_browser_checks) / sizeof (web_head_browser_checks[0]); j++)
    if (type == WebHeadType::Any || type == web_head_browser_checks[j].browsertype) {
      const BrowserCheck &check = web_head_browser_checks[j];
      const std::string path = bp::search_path (check.exename).string();
      if (path.empty()) continue;
      WebHeadBrowser b { .executable = path, .type = check.browsertype };
      const auto& [ex, out, err] = synchronous_exec (b.executable, { "--version" });
      if (ex != 0 || 0 == out.size()) continue;
      WEBHEAD_DEBUG ("%s: %s:\n%s%sexit_code=%d\n", __func__, b.executable.c_str(), out.c_str(), err.c_str(), ex);
      const std::vector<std::string> groups = regex_capture (check.versionpattern, out);
      if (groups.size()) {
        b.identification = groups[0];
        b.version = groups[groups.size() - 1];
        // check for `~/snap/browser/` *after* --version test, so a snap dir has already been created
        if (path_exists (fs::path (home_dir()) / "snap" / check.exename))
          b.snapdir = true;
        browsers.push_back (b);
      }
    }
  return browsers;
}

} // WebHead
