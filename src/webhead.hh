// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#pragma once

#include <memory>
#include <string>
#include <vector>

namespace WebHead {

enum class BrowserType {
  Any,
  Chromium,
  Firefox,
  GoogleChrome,
  Epiphany,
};

struct BrowserInfo {
  std::string executable;
  std::string identification;
  std::string version;
  BrowserType type = BrowserType::Any;
  bool snapdir = false;
};

std::vector<BrowserInfo>     web_head_find (BrowserType type = BrowserType::Any);
std::vector<BrowserInfo>     web_head_sort (const std::vector<BrowserInfo> &browsers);

class WebHeadSession {
public:
  explicit      WebHeadSession  (const std::string &url, const std::string &appname = "");
  int           start           (const BrowserInfo &browser);
  bool          running         ();
  int           kill            (int signal = 1);
  struct Process;
  using ProcessP = std::shared_ptr<Process>;
private:
  std::string    url_, app_;
  ProcessP       process_;
};
using WebHeadSessionP = std::shared_ptr<WebHeadSession>;

} // WebHead
