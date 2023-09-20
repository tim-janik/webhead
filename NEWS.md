## WebHead 0.1.0

### Hardware and System Requirements
* Linux - currently the code is developed and well tested unter Ubuntu Jammy.

### Features
* Introduced browser scanning functionality for Firefow, Epiphany and Chromium type browsers.
* Introduced sorting, so we can give precedence to newer browsers and better integration.
* Added Epiphany web head support using its desktop application mode.
* Added Chromium (and Google-Chrome) web head support using its --bwsi and --app mode.
* Added basic function documentation.
* Added Firefox web head support, by trying to minimize its browser UI.
* Added profile purging logic, so each web head starts with an empty, clean profile.
* Added session and PID tracking (via boost::process) for reliable web head process control.
* Cleaned up the code to use a proper C++ namespace.
