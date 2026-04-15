#----------------------------------------------------------------
#           📌 MY_TESTER.PY 📌
#   - Custom Python-based HTTP tester for default.conf
#   - Automatically validates:
#       • GET / on 8080
#       • POST rejection on /
#       • POST /post_body body size limit
#       • /directory/ index resolution
#       • CGI execution for .bla files
#
#   - Designed to complement the official tester and provide
#     quicker feedback while working on webserv
#----------------------------------------------------------------

#!/usr/bin/env python3

import argparse
import http.client
import socket
import sys

PASS = "\033[92m[PASS]\033[0m"
FAIL = "\033[91m[FAIL]\033[0m"
WARN = "\033[93m[WARN]\033[0m"
INFO = "\033[94m[INFO]\033[0m"

TOTAL = 0
FAILED = 0


def log_ok(msg):
    print(f"{PASS} {msg}")


def log_fail(msg):
    print(f"{FAIL} {msg}")


def log_warn(msg):
    print(f"{WARN} {msg}")


def log_info(msg):
    print(f"{INFO} {msg}")


def check(condition, success_msg, fail_msg):
    global TOTAL, FAILED
    TOTAL += 1
    if condition:
        log_ok(success_msg)
        return True
    FAILED += 1
    log_fail(fail_msg)
    return False


def make_conn(host, port, timeout=3):
    return http.client.HTTPConnection(host, port, timeout=timeout)


def read_response(resp):
    body = resp.read()
    headers = {k.lower(): v for k, v in resp.getheaders()}
    return {
        "status": resp.status,
        "reason": resp.reason,
        "headers": headers,
        "body": body,
        "text": body.decode("utf-8", errors="replace"),
    }


def request(host, port, method, path, body=None, headers=None, timeout=3):
    conn = make_conn(host, port, timeout=timeout)
    try:
        conn.request(method, path, body=body, headers=headers or {})
        resp = conn.getresponse()
        return read_response(resp)
    finally:
        conn.close()


def raw_http(host, port, raw_data, timeout=3):
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.settimeout(timeout)
    sock.connect((host, port))
    sock.sendall(raw_data)
    chunks = []
    try:
        while True:
            data = sock.recv(4096)
            if not data:
                break
            chunks.append(data)
    finally:
        sock.close()
    return b"".join(chunks)


def test_server_is_up(host):
    try:
        resp = request(host, 8080, "GET", "/", timeout=2)
        check(
            resp["status"] in {200, 403, 404, 405},
            "Server is listening on 8080",
            f"Expected an HTTP response on 8080, got status {resp['status']}"
        )
        return True
    except Exception as exc:
        check(
            False,
            "",
            f"Could not connect to {host}:8080 ({exc})"
        )
        return False


def test_get_root(host):
    resp = request(host, 8080, "GET", "/")
    check(
        resp["status"] == 200,
        "GET / on 8080 works",
        f"GET / on 8080 expected 200, got {resp['status']}"
    )


def test_post_root_rejected(host):
    resp = request(host, 8080, "POST", "/")
    check(
        resp["status"] in {403, 405},
        "POST / on 8080 is rejected",
        f"POST / on 8080 expected 403 or 405, got {resp['status']}"
    )


def test_post_body_limit_ok(host):
    body = "a" * 100
    headers = {
        "Content-Type": "text/plain",
        "Content-Length": str(len(body)),
    }
    resp = request(host, 8080, "POST", "/post_body", body=body, headers=headers)
    check(
        resp["status"] not in {413, 500, 502, 503, 504},
        "POST /post_body with 100 bytes is accepted",
        f"POST /post_body with 100 bytes should be accepted, got {resp['status']}"
    )


def test_post_body_limit_too_large(host):
    body = "a" * 101
    headers = {
        "Content-Type": "text/plain",
        "Content-Length": str(len(body)),
    }
    resp = request(host, 8080, "POST", "/post_body", body=body, headers=headers)
    check(
        resp["status"] == 413,
        "POST /post_body with 101 bytes returns 413",
        f"POST /post_body with 101 bytes expected 413, got {resp['status']}"
    )


def test_directory_index(host):
    resp = request(host, 8080, "GET", "/directory/")
    body = resp["text"]
    ok = resp["status"] == 200 and len(body) > 0
    check(
        ok,
        "GET /directory/ resolves directory index",
        f"GET /directory/ expected 200 with content, got status={resp['status']} body={body[:200]!r}"
    )


def test_directory_nested_index(host):
    resp = request(host, 8080, "GET", "/directory/nop/")
    body = resp["text"]
    ok = resp["status"] == 200 and len(body) > 0
    check(
        ok,
        "GET /directory/nop/ resolves nested directory index",
        f"GET /directory/nop/ expected 200 with content, got status={resp['status']} body={body[:200]!r}"
    )


def test_cgi_get_bla(host):
    resp = request(host, 8080, "GET", "/directory/youpi.bla")
    check(
        resp["status"] in {200, 405, 500},
        "GET /directory/youpi.bla returned a coherent CGI-related response",
        f"GET /directory/youpi.bla returned unexpected status {resp['status']}"
    )


def test_cgi_post_bla(host):
    body = "hello=world"
    headers = {
        "Content-Type": "application/x-www-form-urlencoded",
        "Content-Length": str(len(body)),
    }
    resp = request(host, 8080, "POST", "/directory/youpi.bla", body=body, headers=headers)
    check(
        resp["status"] == 200,
        "POST /directory/youpi.bla executes CGI successfully",
        f"POST /directory/youpi.bla expected 200, got {resp['status']} body={resp['text'][:300]!r}"
    )


def test_cgi_post_nested_bla(host):
    body = "abc=123"
    headers = {
        "Content-Type": "application/x-www-form-urlencoded",
        "Content-Length": str(len(body)),
    }
    resp = request(host, 8080, "POST", "/directory/nop/youpi.bad_extension", body=body, headers=headers)
    check(
        resp["status"] in {200, 403, 404, 405},
        "POST to nested non-CGI file returns a coherent status",
        f"POST /directory/nop/youpi.bad_extension returned unexpected status {resp['status']}"
    )


def test_invalid_method(host):
    raw = (
        b"BREW / HTTP/1.1\r\n"
        b"Host: localhost:8080\r\n"
        b"Connection: close\r\n"
        b"\r\n"
    )
    data = raw_http(host, 8080, raw)
    text = data.decode("utf-8", errors="replace")
    ok = "400" in text or "405" in text or "501" in text
    check(
        ok,
        "Invalid method is rejected coherently",
        f"Invalid method got suspicious response:\n{text[:500]}"
    )


def test_bad_request(host):
    raw = b"GARBAGE\r\n\r\n"
    data = raw_http(host, 8080, raw)
    text = data.decode("utf-8", errors="replace")
    ok = "400" in text or "HTTP/" in text
    check(
        ok,
        "Malformed request is handled without hanging",
        f"Malformed request response looks wrong:\n{text[:500]}"
    )


def run_suite(host):
    log_info(f"Running default.conf tests against host={host}")

    if not test_server_is_up(host):
        print()
        print("=" * 60)
        log_fail("Server is not reachable on 8080")
        return 1

    test_get_root(host)
    test_post_root_rejected(host)
    test_post_body_limit_ok(host)
    test_post_body_limit_too_large(host)

    test_directory_index(host)
    test_directory_nested_index(host)

    test_cgi_get_bla(host)
    test_cgi_post_bla(host)
    test_cgi_post_nested_bla(host)

    test_invalid_method(host)
    test_bad_request(host)

    print()
    print("=" * 60)
    if FAILED == 0:
        log_ok(f"All tests passed ({TOTAL}/{TOTAL})")
        return 0
    log_fail(f"{FAILED} tests failed out of {TOTAL}")
    return 1


def main():
    parser = argparse.ArgumentParser(description="default.conf webserv tester")
    parser.add_argument("--host", default="127.0.0.1", help="Server host")
    args = parser.parse_args()
    sys.exit(run_suite(args.host))


if __name__ == "__main__":
    main()