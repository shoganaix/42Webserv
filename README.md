*This project has been created as part of the 42 curriculum by macastro, angnavar, kpineda- and msoriano*

# Webserv
**The objective of this project is to create a simple HTTP web server in C++ that handles multiple clients, supports GET and POST requests, and serves static files.**

> [!NOTE]
> This project is inspired by real-world web servers and aims to teach low-level networking, parsing HTTP requests, and managing concurrency.

# Index
* [Documentation](#documentation)
* [Project Description](#project-description)
* [Features](#features)
* [Methods](#http-methods)
* [Usage](#usage)
* [Project Structure](#project-structure)
* [Final Grade](#grade)

## Documentation
For all the documentation you may need go to our [wiki](https://github.com/shoganaix/42Webserv/wiki)

## Project Description
**Webserv** is a custom HTTP server written in **C++98** capable of handling multiple clients using `poll()`.

The goal of this project is to understand how web servers work internally by implementing the core parts of HTTP request handling, routing, static file serving, CGI execution, uploads, redirections, error handling, and non-blocking I/O using a single event loop mechanism.
Our server is executed with a `.conf` file and supports different server blocks, route-based configuration, CGI by file extension, request body limits, custom error pages, and multiple HTTP methods.
This project was developed as a practical implementation of the concepts behind HTTP servers such as NGINX, while respecting the constraints of the 42 subject.

For more detailed information, refer to the [**subject**](https://github.com/shoganaix/42Webserv/blob/main/en.subject.pdf)

### HTTP Request
An HTTP request consists of a request line, headers, and an optional message body. Here is an example of an HTTP request:

```bash
GET /index.html HTTP/1.1
Host: localhost:8080
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
```

### HTTP Response
An HTTP response also consists of a status line, headers, and an optional message body. Here is an example of an HTTP response:

```bash
HTTP/1.1 200 OK
Content-Type: text/html
Content-Length: 1234

<Message Body>
```

## Features
- **Multiple server** blocks / **ports** ✔️
- Non-blocking sockets with **epoll** ✔️
- **Route matching** ✔️
- **Concurrent connections** ✔️
- **HTTP Protocol: GET, POST, DELETE ...** ✔️
- **Serving Static & Dynamic content** ✔️
- **Error handling** ✔️
- Directory listing with **autoindex** ✔️
- **Redirections** ✔️
- **CGI execution** by file extension ✔️
- **Chunked request** body decoding ✔️
- **Supports cookies and session management** ❌
- **Handles multiple CGI** ❌

### Project-specific behavior
- CGI detection is based on file extension configured through `cgi_ext`
- CGI routing has priority over regular `allow_methods` checks
- As a result, a CGI file can accept `POST` even if its location is declared as `GET`-only
- Chunked request bodies are decoded before being forwarded to CGI
- The server closes connections after sending the response (no keep-alive)


### HTTP Methods
Method        | Description
------------- | -------------
GET ✔️        | Retrieving specific resource or collection of rcs, shouldn't affect data/resource
POST ✔️       | Perform resource-specific processing on the request content
DELETE ✔️     | Remove the specified resource
PUT ❌        | Replace an existing resource or create a new resource at a specific URL
HEAD ❌       | Same as GET, but returns only headers without the body
OPTIONS ❌    | Describe the communication options for the target resource
PATCH ❌      | Apply partial modifications to a resource


## Usage

1. Clone this repository:

```bash
git clone https://github.com/<yourusername>/Webserv
cd Webserv
```

2. Compile and run:

```bash
make
./webserv [Config File] ## leave empty to use the default configuration
```

### Pre-commit

This repository includes a pre-commit setup to keep commits clean and consistent.

#### What it checks
- Trailing whitespace and end-of-file newlines
- Merge conflict markers
- Large files accidentally added
- C/C++ formatting on `src/` and `includes/` using `clang-format`
- C++ compilation with `make` before the commit is accepted

### Install

macOS:

```bash
brew install pre-commit clang-format
pre-commit install
```

Linux (Debian/Ubuntu):

```bash
sudo apt update
sudo apt install -y pre-commit clang-format
pre-commit install
```

### Run manually

```bash
# only staged files
pre-commit run
# specific file
pre-commit run file1.cpp
# all files
pre-commit run --all-files
```

## Project Structure

```text
.
├── Makefile
├── README.md
├── Subject.pdf
├── EvaluationSheet.pdf
├── tester
├── cgi_tester
├── run_tests.sh
├── my_tester.py
├── my_manual_tester.docx
├── Dockerfile
├── docker-readme.md
├── pre-commit-config.yaml
├── .clang-format
├── .gitignore
├── build/
│   └──  src/
│        └──  *.o
├── includes/
│   └──  *.hpp
├── src/
│   └──  *.cpp
├── configs/
│   ├── default.conf
│   ├── my_tester.conf
│   └── nocgi.conf
├── docs/
│   ├── fusion_web/
│   │   ├── index.html
│   │   ├── tours1.html
│   │   └── error_pages/
│   │       └── 404.html
│   └── cgi-bin/
│       └── time.py
├── uploads/
├── YoupiBanane/
└── src/...
```

## Grade
This project was succesfully submitted with a score of 100/100

 <p align="center">
<img width="194" alt="Captura" src="https://github.com/shoganaix/42PushSwap/assets/123943292/a706aec1-2095-45b3-b583-19fbcaf614c9">
</p>

<p align="center">
<sub>NO BONUS</sub>
 </p>
