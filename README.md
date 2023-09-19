# WebHead

WebHead is a small C++17 library that detects, configures, and runs the web rendering engine of pre-installed web browsers
as a chrome-less child process of an application.
By fully owning the web rendering process and controlling its lifetime, this can be used to render the application GUI and
allows front end implementations in pure HTML/CSS/JS.

This project is inspired by:
- [Electron.js](https://www.electronjs.org/) - The most popular web rendering engine for application front ends.
- [Embrace Modern Technology: Using HTML 5 for GUI in C++](https://www.youtube.com/watch?v=bbbcZd4cuxg) by Borislav Stanimirov, providing a good description of the motivations behind implementing UIs based on a web engine.
- [WebUI v2](https://github.com/webui-dev/webui) - Similar in scope with MacOS & Windows support and includes a Civetweb WebSocket implementation.

Goals:
- A parent->child process relationship must be maintained between the application and the web head.
- The web head runtime must be reliably coupled to the existence of the child process.
- The web heads must be chrome-less without any ordinary web-browser menus or popups.

## Roadmap

- [x] Scan for available browsers.
- [x] Add Chromium type web heads.
- [x] Add Firefox web head (needs profile setup).
- [x] Add Epiphany web head (needs desktop file setup).
- [ ] Support a user provided Electron as web head.
- [ ] Support a user provided NW.js as web head.
- [ ] Support size optimized WebKit based renderer.
- [ ] Example: Demonstrate JSONIPC setup.
- [ ] Example: Add pandoc based mdview example.
- [ ] Security: Add example that allows single-connection only.
- [ ] Security: Avoid proxies to disallow sniffing.
