# Jacob's Matrix web port

This branch ports release 004 to Emscripten WebAssembly while preserving the
game's threaded engine and fire effects.  The build uses `-pthread`, a
four-worker pool, `SharedArrayBuffer`, and `PROXY_TO_PTHREAD`; it is not a
single-thread fallback.

## Build and run locally

Install and activate Emscripten 6.0.6, then run:

```sh
make -C web
make -C web serve
```

Open <http://127.0.0.1:8000>.  `serve.py` sends COOP/COEP headers so that a
normal local build is immediately cross-origin isolated.

## GitHub Pages

GitHub Pages does not offer per-repository response-header configuration.  The
site therefore vendors `coi-serviceworker` v0.1.7 from commit
`7b1d2a092d0d2dd2b7270b6f12f13605de26f214`.  On a first visit it installs a
same-origin service worker and reloads once.  The worker adds COOP/COEP to the
local game files before `index.js` starts, enabling `SharedArrayBuffer` and
WebAssembly pthreads.

Private browsing modes that disable service workers cannot run this threaded
build.  Current Chrome, Edge, and Firefox releases are the intended targets.
Saved progress is stored in IndexedDB through Emscripten's IDBFS mount.

Pushes to `release-004-web` are built and deployed by
`.github/workflows/pages.yml`.
