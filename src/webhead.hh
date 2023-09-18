// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#pragma once

#include <string>
#include <vector>

namespace WebHead {

enum class WebHeadType {
  Any,
  Chromium,
  Firefox,
  GoogleChrome,
  Epiphany,
};

struct WebHeadBrowser {
  std::string executable;
  std::string identification;
  std::string version;
  WebHeadType type = WebHeadType::Any;
  bool snapdir = false;
};

std::vector<WebHeadBrowser>     web_head_find (WebHeadType type = WebHeadType::Any);

} // WebHead
