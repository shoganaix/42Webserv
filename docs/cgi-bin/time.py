#!/usr/bin/env python3
import os
import datetime

body = []
body.append("CGI OK")
body.append("REQUEST_METHOD=" + os.environ.get("REQUEST_METHOD", ""))
body.append("SCRIPT_NAME=" + os.environ.get("SCRIPT_NAME", ""))
body.append("QUERY_STRING=" + os.environ.get("QUERY_STRING", ""))
body.append("CONTENT_LENGTH=" + os.environ.get("CONTENT_LENGTH", ""))
body.append("TIME=" + datetime.datetime.utcnow().isoformat() + "Z")
content = "\n".join(body) + "\n"

print("Status: 200 OK")
print("Content-Type: text/plain")
print("Content-Length: {}".format(len(content)))
print()
print(content, end="")
