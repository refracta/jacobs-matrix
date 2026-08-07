#!/usr/bin/env python3
"""Serve the web build with the headers required by WebAssembly threads."""

import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer


class CrossOriginIsolatedHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Resource-Policy", "cross-origin")
        super().end_headers()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--directory", default="dist")
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8000)
    args = parser.parse_args()

    handler = partial(CrossOriginIsolatedHandler, directory=args.directory)
    server = ThreadingHTTPServer((args.bind, args.port), handler)
    print("Serving http://%s:%d" % (args.bind, args.port))
    server.serve_forever()


if __name__ == "__main__":
    main()
