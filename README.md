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
* [Final Grade](#grade)

## Documentation
For all the documentation you may need go to our [wiki](https://github.com/shoganaix/42Webserv/wiki)

## Project Description
Webserv is a C++ HTTP server capable of handling multiple clients using `poll()`.  
It parses HTTP requests according to the standard, serves static files, handles error pages, and can implement multiple server configurations using a `.conf` file.  
HTTP Message can be either a request or response.

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

---

## Features
- **Concurrent connections** ✔️
- **HTTP Protocol: GET, POST, DELETE ...** ✔️
- **Serving Static & Dynamic content** ✔️
- **Suppports Config files** ✔️
- **Error handling** ✔️
- **Supports cookies and session management** ❌
- **Handles multiple CGI** ❌

---
## HTTP Methods
Method        | Description  
------------- | -------------
GET           | Retrieve a specific resource or a collection of resources, should not affect the data/resource
POST          | Perform resource-specific processing on the request content
PUT           | Replace an existing resource or create a new resource at a specific URL
DELETE        | Remove the specified resource
HEAD          | Same as GET, but returns only headers without the body
OPTIONS       | Describe the communication options for the target resource
PATCH         | Apply partial modifications to a resource

---
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

## Grade

 <p align="center">
<img width="194" alt="Captura" src="https://github.com/shoganaix/42PushSwap/assets/123943292/a706aec1-2095-45b3-b583-19fbcaf614c9">
</p>

<p align="center">
<sub>NO BONUS</sub>
 </p>
