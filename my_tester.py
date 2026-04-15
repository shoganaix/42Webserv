#!/usr/bin/env python3

import argparse
import http.client
import socket
import sys
import time
import threading

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


def make_conn(host, port, timeout=4):
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


def request(host, port, method, path, body=None, headers=None, timeout=4):
    conn = make_conn(host, port, timeout=timeout)
    try:
        conn.request(method, path, body=body, headers=headers or {})
        resp = conn.getresponse()
        return read_response(resp)
    finally:
        conn.close()


def raw_http(host, port, raw_data, timeout=4):
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
        check(False, "", f"Could not connect to {host}:8080 ({exc})")
        return False


def test_get_root(host):
    resp = request(host, 8080, "GET", "/")
    body = resp["text"]
    check(
        resp["status"] == 200 and "<html" in body.lower(),
        "GET / returns the main HTML page",
        f"GET / expected 200 with HTML content, got status={resp['status']} body={body[:200]!r}"
    )


def test_post_root(host):
    resp = request(host, 8080, "POST", "/", body="hello", headers={
        "Content-Type": "text/plain",
        "Content-Length": "5",
    })
    check(
        resp["status"] in {200, 201, 202, 204, 405},
        "POST / returns a coherent status for manual_test.conf",
        f"POST / returned unexpected status {resp['status']} body={resp['text'][:200]!r}"
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


def test_tours_get(host):
    resp = request(host, 8080, "GET", "/tours")
    body = resp["text"]
    check(
        resp["status"] == 200 and ("tour" in body.lower() or "<html" in body.lower()),
        "GET /tours works",
        f"GET /tours expected 200 with tours content, got status={resp['status']} body={body[:200]!r}"
    )


def test_tours_get_slash(host):
    resp = request(host, 8080, "GET", "/tours/")
    check(
        resp["status"] in {200, 301, 302, 307, 308, 404},
        "GET /tours/ returns a coherent status",
        f"GET /tours/ returned unexpected status {resp['status']} body={resp['text'][:200]!r}"
    )


def test_tours_post(host):
    body = "hello=tours"
    headers = {
        "Content-Type": "application/x-www-form-urlencoded",
        "Content-Length": str(len(body)),
    }
    resp = request(host, 8080, "POST", "/tours", body=body, headers=headers)
    check(
        resp["status"] in {200, 201, 202, 204, 405},
        "POST /tours returns a coherent status",
        f"POST /tours returned unexpected status {resp['status']} body={resp['text'][:200]!r}"
    )


def test_redirect(host):
    resp = request(host, 8080, "GET", "/red")
    has_location = "location" in resp["headers"]
    check(
        resp["status"] in {301, 302, 303, 307, 308} and has_location,
        "GET /red returns a redirect with Location header",
        f"GET /red expected redirect with Location header, got status={resp['status']} headers={resp['headers']}"
    )


def test_404_page(host):
    resp = request(host, 8080, "GET", "/this-route-does-not-exist")
    body = resp["text"]
    check(
        resp["status"] == 404 and ("404" in body or "no existe" in body.lower()),
        "Custom 404 page works",
        f"Expected 404 custom page, got status={resp['status']} body={body[:200]!r}"
    )


def test_directory_index(host):
    resp = request(host, 8080, "GET", "/directory/")
    check(
        resp["status"] == 200,
        "GET /directory/ returns 200",
        f"GET /directory/ expected 200, got status={resp['status']} body={resp['text'][:200]!r}"
    )


def test_directory_nested_index(host):
    resp = request(host, 8080, "GET", "/directory/nop/")
    check(
        resp["status"] == 200,
        "GET /directory/nop/ returns 200",
        f"GET /directory/nop/ expected 200, got status={resp['status']} body={resp['text'][:200]!r}"
    )


def test_directory_direct_file(host):
    resp = request(host, 8080, "GET", "/directory/youpi.bad_extension")
    check(
        resp["status"] in {200, 404},
        "GET /directory/youpi.bad_extension returns a coherent status",
        f"GET /directory/youpi.bad_extension returned unexpected status {resp['status']}"
    )


def test_cgi_get(host):
    resp = request(host, 8080, "GET", "/cgi-bin/time.py")
    body = resp["text"]
    ok = (
        resp["status"] == 200 and
        ("CGI OK" in body or "REQUEST_METHOD=GET" in body)
    )
    check(
        ok,
        "GET /cgi-bin/time.py executes CGI",
        f"GET /cgi-bin/time.py failed. status={resp['status']} body={body[:300]!r}"
    )


def test_cgi_get_query(host):
    resp = request(host, 8080, "GET", "/cgi-bin/time.py?name=maria&x=1")
    body = resp["text"]
    ok = (
        resp["status"] == 200 and
        ("QUERY_STRING=name=maria&x=1" in body or "CGI OK" in body)
    )
    check(
        ok,
        "GET /cgi-bin/time.py with query string works",
        f"CGI query string failed. status={resp['status']} body={body[:300]!r}"
    )


def test_cgi_post(host):
    body = "hello=world&x=42"
    headers = {
        "Content-Type": "application/x-www-form-urlencoded",
        "Content-Length": str(len(body)),
    }
    resp = request(host, 8080, "POST", "/cgi-bin/time.py", body=body, headers=headers)
    text = resp["text"]
    ok = (
        resp["status"] == 200 and
        ("REQUEST_METHOD=POST" in text or "CGI OK" in text)
    )
    check(
        ok,
        "POST /cgi-bin/time.py executes CGI successfully",
        f"POST /cgi-bin/time.py failed. status={resp['status']} body={text[:300]!r}"
    )


def test_cgi_delete(host):
    resp = request(host, 8080, "DELETE", "/cgi-bin/time.py")
    check(
        resp["status"] in {200, 403, 405, 501},
        "DELETE /cgi-bin/time.py returns a coherent status",
        f"DELETE /cgi-bin/time.py returned unexpected status {resp['status']} body={resp['text'][:200]!r}"
    )


def test_chunked_request(host):
    raw = (
        b"POST /cgi-bin/time.py HTTP/1.1\r\n"
        b"Host: localhost:8080\r\n"
        b"Transfer-Encoding: chunked\r\n"
        b"Content-Type: text/plain\r\n"
        b"\r\n"
        b"5\r\nhello\r\n"
        b"0\r\n\r\n"
    )
    data = raw_http(host, 8080, raw, timeout=5)
    text = data.decode("utf-8", errors="replace")
    ok = "200 OK" in text or "CGI OK" in text or "REQUEST_METHOD=POST" in text
    check(
        ok,
        "Chunked POST to CGI does not crash and returns a valid response",
        f"Chunked POST seems broken. Raw response:\n{text[:500]}"
    )


def test_delete_existing_file(host):
    resp = request(host, 8080, "DELETE", "/tours1.html")
    check(
        resp["status"] in {200, 204, 404, 405},
        "DELETE /tours1.html returns a coherent status",
        f"DELETE /tours1.html returned unexpected status {resp['status']} body={resp['text'][:200]!r}"
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


def one_client_get(host, path, results, idx):
    try:
        resp = request(host, 8080, "GET", path, timeout=5)
        results[idx] = resp["status"]
    except Exception as exc:
        results[idx] = f"EXC:{exc}"


def stress_test(host, n=20):
    threads = []
    results = [None] * n
    start = time.time()

    for i in range(n):
        path = "/" if i % 2 == 0 else "/cgi-bin/time.py"
        t = threading.Thread(target=one_client_get, args=(host, path, results, i))
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    elapsed = time.time() - start
    success = sum(1 for x in results if x == 200)

    check(
        success >= int(n * 0.7),
        f"Stress test: {success}/{n} requests returned 200 in {elapsed:.2f}s",
        f"Stress test weak result: {success}/{n} successful in {elapsed:.2f}s. Results={results}"
    )


def run_suite(host):
    log_info(f"Running manual_test.conf tests against host={host}")

    if not test_server_is_up(host):
        print()
        print("=" * 60)
        log_fail("Server is not reachable on 8080")
        return 1

    test_get_root(host)
    test_post_root(host)

    test_post_body_limit_ok(host)
    test_post_body_limit_too_large(host)

    test_tours_get(host)
    test_tours_get_slash(host)
    test_tours_post(host)

    test_redirect(host)
    test_404_page(host)

    test_directory_index(host)
    test_directory_nested_index(host)
    test_directory_direct_file(host)

    test_cgi_get(host)
    test_cgi_get_query(host)
    test_cgi_post(host)
    test_cgi_delete(host)

    test_chunked_request(host)
    test_delete_existing_file(host)

    test_invalid_method(host)
    test_bad_request(host)

    stress_test(host, n=20)

    print()
    print("=" * 60)
    if FAILED == 0:
        log_ok(f"All tests passed ({TOTAL}/{TOTAL})")
        return 0
    log_fail(f"{FAILED} tests failed out of {TOTAL}")
    return 1


def main():
    parser = argparse.ArgumentParser(description="manual_test.conf webserv tester")
    parser.add_argument("--host", default="127.0.0.1", help="Server host")
    args = parser.parse_args()
    sys.exit(run_suite(args.host))


if __name__ == "__main__":
    main()