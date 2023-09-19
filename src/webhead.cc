// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#include "webhead.hh"
#include <stdlib.h>
#include <locale.h>
#include <stdarg.h>
#include <sys/time.h>
#include <boost/process.hpp>
#include <filesystem>
#include <regex>

#define WEBHEAD_DEBUG(...)      do { if (0) dprintf (2, __VA_ARGS__); } while (0)

namespace WebHead {

static std::string posix_printf (const char*, ...) __attribute__ ((__format__ (__printf__, 1, 2)));

/// Join a vector of strings.
static std::string
string_join (const std::string &junctor, const std::vector<std::string> &strvec)
{
  std::string s = strvec.size() ? strvec[0] : "";
  for (size_t i = 1; i < strvec.size(); i++)
    s += junctor + strvec[i];
  return s;
}

/// Create std::string from printf() format style in the Posix C locale.
static std::string
posix_printf (const char *format, ...)
{
  va_list args;
  static locale_t posix_c_locale = newlocale (LC_ALL_MASK, "C", NULL);
  locale_t saved_locale = uselocale (posix_c_locale);
  va_start (args, format);
  char buffer[8192] = { 0, };
  vsnprintf (buffer, sizeof (buffer) - 1, format, args);
  buffer[sizeof (buffer)-1] = 0;
  va_end (args);
  uselocale (saved_locale);
  return buffer;
}

/// Yield gethostname() as std::string
static const char*
host_name()
{
  static char buffer[256] = { 0, };
  if (!buffer[0]) {
    gethostname (buffer, sizeof (buffer));
    buffer[sizeof (buffer)-1] = 0;
  }
  return buffer;
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

/// Create directory, including parents.
static bool
path_mkdirs (const std::filesystem::path &path)
{
  std::error_code ec{};
  std::filesystem::create_directories (path, ec);
  return path_exists (path);
}

/// Create a file from a string.
static void
write_string (const std::string &filename, const std::string &contents)
{
  std::ofstream ofile (filename);
  ofile << contents;
  ofile.close();
}

/// Path of the current users home directory
static std::string
home_dir()
{
  return getenv ("HOME");
}

/// Get the $XDG_CACHE_HOME directory, see: https://specifications.freedesktop.org/basedir-spec/latest
static std::string
cache_home ()
{
  namespace fs = std::filesystem;
  const char *var = getenv ("XDG_CACHE_HOME");
  if (var && fs::path (var).is_absolute())
    return var;
  return fs::path (home_dir()) / ".cache";
}

/// Return the current time as uint64 in µseconds.
static long long unsigned
timestamp_realtime ()
{
  struct timeval now = { 0, 0 };
  gettimeofday (&now, NULL);
  return now.tv_sec * 1000000ULL + now.tv_usec;
}

/// Create new temporary dir and purge old dirs of the same kind.
static std::string
create_hostpid_subdir (const std::string &parentdir, bool clean_stale_siblings)
{
  namespace fs = std::filesystem;
  // determine dirname for current and previous sessions
  const std::string prefix = posix_printf ("%s-%08lx-", host_name(), gethostid());
  if (clean_stale_siblings) {
    std::vector<std::string> others;
    std::error_code ec{};
    for (const auto &entry : fs::directory_iterator (parentdir, ec)) {
      // find previous session with stale PID in directory name
      const std::string sibling = entry.path().filename();
      if (sibling.compare (0, prefix.size(), prefix) != 0) continue;
      char *endptr = nullptr;
      const size_t sibling_pid = strtoul (sibling.c_str() + prefix.size(), &endptr, 10);
      if (sibling_pid <= 0 || (endptr && *endptr)) continue;
      if (kill (sibling_pid, 0) == -1 && errno == ESRCH) {
        WEBHEAD_DEBUG ("%s: cleaning stale temp dir: %s\n", __func__, entry.path().c_str());
        std::error_code ec;
        std::filesystem::remove_all (entry.path(), ec);
      }
    }
  }
  // create directory with PID of the current session
  const std::string host_dir = fs::path (parentdir) / (prefix + posix_printf ("%u", getpid()));
  if (path_exists (host_dir)) {
    errno = EEXIST;
    return "";
  }
  return path_mkdirs (host_dir) ? host_dir : "";
}

// Create suitable temporary WebHead directory, take snap R/W limitations into account.
static std::string
create_webhead_tempdir (const std::string &executable, const std::string &appname, bool forsnap)
{
  namespace fs = std::filesystem;
  std::string basedir, exename = fs::path (executable).filename();
  // Many snap apps can only write under ~/snap/<self>/current/
  if (forsnap)
    basedir = fs::path (home_dir()) / "snap" / exename / "current" / "WebHead";
  else
    basedir = fs::path (cache_home()) / "WebHead";
  const std::string runtimedir = create_hostpid_subdir (basedir, true); // ~/.../WebHead/hostname-aabbccdd-123
  std::string subdir = forsnap ? "" : exename + "-";
  subdir += posix_printf ("%llu", timestamp_realtime());
  const fs::path tempdir = fs::path (runtimedir) / subdir;
  return !path_exists (tempdir) && path_mkdirs (tempdir) ? tempdir : "";
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
  // "google-chrome-stable", "google-chrome-beta", "google-chrome-unstable" have one canonical alias, "google-chrome"
  { "chromium",                 "(Chromium\\s\\s*)([0-9]+[-0-9.a-z+]*).*",                WebHeadType::Chromium },
  // "chromium-browser", is a wrapper, so cannot be detected as ~/snap/chromium-browser
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
        // check for `~/snap/browser/` *after* --version test, so a ~/snap/<self>/current/ dir has already been created
        if (path_exists (fs::path (home_dir()) / "snap" / check.exename))
          b.snapdir = true;
        browsers.push_back (b);
      }
    }
  return web_head_sort (browsers);
}

struct WebHeadBrowserLesser {
  bool
  operator() (const WebHeadBrowser &a, const WebHeadBrowser &b) const
  {
    if (a.type != b.type)
      return a.type < b.type;
    if (a.version != b.version) // give precedence to higher versions
      return strverscmp (a.version.c_str(), b.version.c_str()) > 0;
    if (a.identification != b.identification)
      return strcmp (a.identification.c_str(), b.identification.c_str()) < 0;
    if (a.snapdir != b.snapdir)
      return a.snapdir < b.snapdir;
    if (a.executable != b.executable)
      return a.executable < b.executable;
    return false;
  }
};

/// Sort browser list by type, version, etc.
std::vector<WebHeadBrowser>
web_head_sort (const std::vector<WebHeadBrowser> &browsers)
{
  std::vector<WebHeadBrowser> browservector = browsers;
  std::stable_sort (browservector.begin(), browservector.end(), WebHeadBrowserLesser());
  return browservector;
}

/// WebHeadSession::Process simply wraps boost::process::child.
struct WebHeadSession::Process {
  boost::process::child child = {};
};

/// Write generic files to browser profile
static void
create_profile_files (const std::string &profiledir, const std::string &exename, bool snapdir, const std::string &url, const std::string appname)
{
  namespace fs = std::filesystem;
  const auto s =
    posix_printf ("WebHead directory to host temporary profile:\n\nApplication=%s\nWebHead=%s\n", appname.c_str(), exename.c_str());
  write_string (fs::path (profiledir) / "WebHead.txt", s);
}

/// Start chromium type browsers
static WebHeadSession::ProcessP
start_chromium (const std::string &executable, bool snapdir, const std::string &url, const std::string appname)
{
  namespace fs = std::filesystem;
  namespace bp = boost::process;
  const std::string exename = fs::path (executable).filename();
  const std::string pdir = create_webhead_tempdir (executable, appname, snapdir);
  if (pdir == "") return nullptr;
  create_profile_files (pdir, exename, snapdir, url, appname);
  const std::string logfile = fs::path (pdir) / "WebHead.log";
  // https://www.chromium.org/developers/how-tos/run-chromium-with-flags/
  // https://peter.sh/experiments/chromium-command-line-switches/
  std::vector<std::string> args = {
    "--user-data-dir=" + pdir,          // Avoids "Opening in existing browser session"
    "--incognito",
    "--no-first-run",                   // Avoid popup for empty profile
    "--no-experiments",
    "--no-default-browser-check",
    "--disable-extensions",
    "--disable-sync",
    "--bwsi",
    "--new-window",
    "--app=" + url,
  };
  WEBHEAD_DEBUG ("%s: %s %s\n", __func__, executable.c_str(), string_join (" ", args).c_str());
  WebHeadSession::ProcessP pp = std::make_shared<WebHeadSession::Process>();
  std::error_code ec{};
  pp->child = bp::child (executable, bp::args (args), (bp::std_err & bp::std_out) > logfile, bp::std_in < bp::null, ec);
  errno = ec.value();
  return pp;
}

/// Prepare web head session
WebHeadSession::WebHeadSession (const std::string &url, const std::string &appname) :
  url_ (url), app_ (appname)
{
  if (app_.empty()) {
    char buf[4096] = { 0, };
    ssize_t n = ::readlink ("/proc/self/exe", buf, sizeof (buf) - 1);
    app_ = n > 0 ? buf : "/proc/self/";
  }
}

/// Start web head with the given `url` in `browser`, returns errno.
int
WebHeadSession::start (const WebHeadBrowser &browser)
{
  if (process_) { WEBHEAD_DEBUG ("%s: session already started", __func__); return EINVAL; }
  switch (browser.type)
    {
    case WebHeadType::Chromium:
    case WebHeadType::GoogleChrome:
      process_ = start_chromium (browser.executable, browser.snapdir, url_, app_);
      if (process_ && process_->child.running())
        errno = 0;
      else if (process_) {
        const int last = errno;
        std::error_code ec{};
        process_->child.terminate (ec);
        process_->child.wait();
        process_ = nullptr;
        errno = last ? last : EIO;
      }
      break;
    case WebHeadType::Firefox:
    case WebHeadType::Epiphany:
    case WebHeadType::Any:
      errno = ENOSYS;
      break;
    }
  return errno;
}

/// Check if the web head is still running.
bool
WebHeadSession::running ()
{
  return process_ && process_->child.running();
}

/// Kill the web head with a signal if it is still running, returns errno.
int
WebHeadSession::kill (int signal)
{
  if (!running()) return ESRCH;
  const int err = ::kill (process_->child.id(), signal);
  const int last_errno = err < 0 && !errno ? EINVAL : errno;
  WEBHEAD_DEBUG ("%s: killed, signal=%d, pid=%d: %s\n", __func__, signal, process_->child.id(), strerror (errno));
  return last_errno;
}

} // WebHead
