#----------------------------------------------------------------
#                     📌 CGI SCRIPT 📌
# ◉ cgi-bin/time.py
#   - Python CGI script used to validate CGI exec
#   - Displays:
#       • Request method
#       • Query string
#       • Content length
#       • Server environment variables
#       • Request body (for POST requests)
#----------------------------------------------------------------

#!/usr/bin/env python3
import os
import sys
import datetime

def get_body():
    try:
        content_length = os.environ.get("CONTENT_LENGTH", "")
        if content_length.isdigit():
            return sys.stdin.read(int(content_length))
        return sys.stdin.read()
    except Exception as e:
        return f"[body read error: {e}]"

body_in = get_body()

lines = [
    "CGI OK",
    "TIME=" + datetime.datetime.utcnow().isoformat() + "Z",
    "REQUEST_METHOD=" + os.environ.get("REQUEST_METHOD", ""),
    "SCRIPT_NAME=" + os.environ.get("SCRIPT_NAME", ""),
    "PATH_INFO=" + os.environ.get("PATH_INFO", ""),
    "QUERY_STRING=" + os.environ.get("QUERY_STRING", ""),
    "CONTENT_TYPE=" + os.environ.get("CONTENT_TYPE", ""),
    "CONTENT_LENGTH=" + os.environ.get("CONTENT_LENGTH", ""),
    "SERVER_PROTOCOL=" + os.environ.get("SERVER_PROTOCOL", ""),
    "REQUEST_URI=" + os.environ.get("REQUEST_URI", ""),
    "BODY_START",
    body_in,
    "BODY_END",
]

content = "\n".join(lines) + "\n"

print("Status: 200 OK")
print("Content-Type: text/plain; charset=utf-8")
print("Content-Length: {}".format(len(content.encode("utf-8"))))
print()
print(content, end="")