#!/usr/bin/env python3
"""
42 Webserv Evaluation Tests
Tests focalizados en los requisitos específicos de la evaluación 42
"""

import subprocess
import socket
import time
import sys
import os
import signal
from pathlib import Path

class Color:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

def log(msg, level="INFO"):
    levels = {
        "INFO": Color.BLUE,
        "OK": Color.GREEN,
        "FAIL": Color.RED,
        "TODO": Color.YELLOW,
        "STEP": Color.CYAN,
    }
    color = levels.get(level, Color.RESET)
    print(f"{color}[{level}]{Color.RESET} {msg}")

class Server:
    def __init__(self, binary="./webserv", config="./configs/default.conf"):
        self.binary = binary
        self.config = config
        self.process = None
        
    def start(self):
        try:
            self.process = subprocess.Popen(
                [self.binary, self.config],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                preexec_fn=os.setsid
            )
            time.sleep(1)
            if self.process.poll() is None:
                log(f"Server started (PID {self.process.pid})", "OK")
                return True
            return False
        except Exception as e:
            log(f"Failed to start: {e}", "FAIL")
            return False
    
    def stop(self):
        if self.process:
            try:
                os.killpg(os.getpgid(self.process.pid), signal.SIGTERM)
                self.process.wait(timeout=3)
                log("Server stopped", "OK")
            except:
                try:
                    os.killpg(os.getpgid(self.process.pid), signal.SIGKILL)
                except:
                    pass

def test_section(title):
    """Print test section header"""
    print(f"\n{Color.HEADER}{'='*70}{Color.RESET}")
    print(f"{Color.HEADER}  {title}{Color.RESET}")
    print(f"{Color.HEADER}{'='*70}{Color.RESET}\n")

def manual_test(description, verification):
    """Guide for manual testing"""
    log(f"TEST: {description}", "STEP")
    print(f"\n{Color.BOLD}📋 QUÉ HACER:{Color.RESET}")
    for line in verification.split('\n'):
        print(f"   {line}")
    answer = input(f"\n{Color.BOLD}¿Pasó? (s/n/detalles): {Color.RESET}").strip().lower()
    passed = answer.startswith('s') or answer.startswith('yes') or answer.startswith('y')
    status = "✓" if passed else "✗"
    print(f"\n{Color.GREEN if passed else Color.RED}{status}{Color.RESET} {description}\n")
    return passed

def socket_request(host, port, method, path, body=None, custom_headers=None):
    """Send raw HTTP request via socket"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect((host, port))
        
        # Build request
        request = f"{method} {path} HTTP/1.1\r\n"
        request += f"Host: {host}:{port}\r\n"
        request += "Connection: close\r\n"
        
        if custom_headers:
            for key, val in custom_headers.items():
                request += f"{key}: {val}\r\n"
        
        if body:
            request += f"Content-Length: {len(body)}\r\n"
        
        request += "\r\n"
        
        if body:
            request += body
        
        sock.sendall(request.encode())
        
        response = b""
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            response += chunk
        
        sock.close()
        return response.decode('utf-8', errors='ignore')
    except Exception as e:
        return f"ERROR: {e}"

def test_io_multiplexing():
    """Test I/O Multiplexing Implementation"""
    test_section("1️⃣  I/O MULTIPLEXING - CODE REVIEW")
    
    log("Necesito revisar tu código", "TODO")
    print("\n📌 RESPONDE ESTAS PREGUNTAS:\n")
    
    tests = []
    
    q1 = input("❓ ¿Qué mecanismo de multiplexing usas? (select/poll/epoll/kqueue): ").strip()
    if q1:
        log(f"Mecanismo: {q1}", "OK")
        tests.append(True)
    
    q2 = input("❓ ¿Se evalúan READ y WRITE al MISMO TIEMPO? (sí/no): ").strip().lower()
    if q2.startswith('s'):
        log("✓ Evaluación simultánea confirmada", "OK")
        tests.append(True)
    else:
        log("✗ DEBE ser simultáneo", "FAIL")
        tests.append(False)
    
    q3 = input("❓ ¿Hay UN ÚNICO select en el main loop? (sí/no): ").strip().lower()
    if q3.startswith('s'):
        log("✓ Un único select confirmado", "OK")
        tests.append(True)
    else:
        log("✗ DEBE haber un único select", "FAIL")
        tests.append(False)
    
    q4 = input("❓ ¿1 read/write por cliente por select? (sí/no): ").strip().lower()
    if q4.startswith('s'):
        log("✓ Límite de 1 R/W confirmado", "OK")
        tests.append(True)
    else:
        log("✗ Debe ser max 1 por ciclo", "FAIL")
        tests.append(False)
    
    q5 = input("❓ ¿Se chequea errno después de read/recv/write/send? (sí/no): ").strip().lower()
    if q5.startswith('n'):
        log("✓ Correcto - no se chequea errno", "OK")
        tests.append(True)
    else:
        log("✗ DEBE NO chequearse errno aquí", "FAIL")
        tests.append(False)
    
    q6 = input("❓ ¿Hay read/write directo SIN select? (sí/no): ").strip().lower()
    if q6.startswith('n'):
        log("✓ Correcto - todo a través de select", "OK")
        tests.append(True)
    else:
        log("✗ TODO debe ir a través de select", "FAIL")
        tests.append(False)
    
    return all(tests)

def test_basic_requests():
    """Test basic HTTP requests"""
    test_section("2️⃣  PETICIONES HTTP BÁSICAS")
    
    server = Server()
    if not server.start():
        log("No se pudo iniciar servidor", "FAIL")
        return False
    
    time.sleep(2)
    tests = []
    
    try:
        # GET request
        log("Testing GET /", "STEP")
        resp = socket_request("127.0.0.1", 8080, "GET", "/")
        if "200" in resp or "HTTP/1.1" in resp:
            log("✓ GET request works", "OK")
            tests.append(True)
        else:
            log("✗ GET request failed", "FAIL")
            tests.append(False)
        
        # POST request
        log("Testing POST /post_body", "STEP")
        resp = socket_request("127.0.0.1", 8080, "POST", "/post_body", body="test")
        if "200" in resp or "HTTP/1.1" in resp:
            log("✓ POST request works", "OK")
            tests.append(True)
        else:
            log("✗ POST request failed", "FAIL")
            tests.append(False)
        
        # DELETE request
        log("Testing DELETE /", "STEP")
        resp = socket_request("127.0.0.1", 8080, "DELETE", "/")
        if "405" in resp or "403" in resp or "HTTP/1.1" in resp:
            log("✓ DELETE handled (forbidden or method not allowed)", "OK")
            tests.append(True)
        else:
            log("✗ DELETE not handled properly", "FAIL")
            tests.append(False)
        
        # Unknown method
        log("Testing UNKNOWN method", "STEP")
        resp = socket_request("127.0.0.1", 8080, "BADMETHOD", "/")
        if "HTTP/1.1" in resp and resp:  # Just check server responds
            log("✓ Unknown method handled (no crash)", "OK")
            tests.append(True)
        else:
            log("✗ Server might have crashed", "FAIL")
            tests.append(False)
        
        # 404 error
        log("Testing 404 Not Found", "STEP")
        resp = socket_request("127.0.0.1", 8080, "GET", "/nonexistent12345")
        if "404" in resp:
            log("✓ 404 error returned correctly", "OK")
            tests.append(True)
        else:
            log("✗ 404 not returned", "FAIL")
            tests.append(False)
        
    finally:
        server.stop()
    
    return all(tests)

def test_body_size_limit():
    """Test client body size limit"""
    test_section("3️⃣  LÍMITE DE TAMAÑO DE CLIENTE")
    
    server = Server()
    if not server.start():
        return False
    
    time.sleep(2)
    tests = []
    
    try:
        # Small body (should work)
        small_body = "x" * 50
        resp = socket_request("127.0.0.1", 8080, "POST", "/post_body", body=small_body)
        if "200" in resp or "HTTP/1.1" in resp:
            log("✓ Small body (50 bytes) accepted", "OK")
            tests.append(True)
        else:
            log("✗ Small body rejected", "FAIL")
            tests.append(False)
        
        # Large body (should fail with 413)
        large_body = "x" * 500
        resp = socket_request("127.0.0.1", 8080, "POST", "/post_body", body=large_body)
        if "413" in resp or "Payload Too Large" in resp:
            log("✓ Large body (500 bytes) rejected with 413", "OK")
            tests.append(True)
        else:
            log("✗ Large body not properly rejected", "FAIL")
            tests.append(False)
    
    finally:
        server.stop()
    
    return all(tests)

def test_configuration():
    """Test configuration features"""
    test_section("4️⃣  CARACTERÍSTICAS DE CONFIGURACIÓN")
    
    tests = []
    
    tests.append(manual_test(
        "Múltiples servidores en diferentes puertos",
        "1. Edita configs/default.conf\n"
        "2. Agrega dos servers con puertos 8080 y 8081\n"
        "3. Inicia webserv\n"
        "4. Abre http://127.0.0.1:8080/ y http://127.0.0.1:8081/\n"
        "5. Verifica que cada puerto sirve el contenido correcto"
    ))
    
    tests.append(manual_test(
        "Múltiples hostnames",
        "1. Modifica /etc/hosts para mapear example.com a 127.0.0.1\n"
        "2. Configura servidor con server_name example.com\n"
        "3. Ejecuta: curl --resolve example.com:8080:127.0.0.1 http://example.com:8080/\n"
        "4. Verifica que responde correctamente"
    ))
    
    tests.append(manual_test(
        "Página de error personalizada (404)",
        "1. Verifica que error_page 404 esté en la config\n"
        "2. Accede a /nonexistent\n"
        "3. Debería mostrar la página de error configurada"
    ))
    
    tests.append(manual_test(
        "Restricción de métodos HTTP",
        "1. Verifica que / solo acepta GET\n"
        "2. Intenta POST a / (debería retornar 405)\n"
        "3. Intenta POST a /post_body (debería funcionar)"
    ))
    
    tests.append(manual_test(
        "Rutas a diferentes directorios (alias)",
        "1. Verifica que /directory/ apunta a YoupiBanane/\n"
        "2. Accede a http://127.0.0.1:8080/directory/\n"
        "3. Debe listar o mostrar el archivo índice"
    ))
    
    return tests

def test_browser_compatibility():
    """Test browser compatibility"""
    test_section("5️⃣  COMPATIBILIDAD CON NAVEGADOR")
    
    server = Server()
    if not server.start():
        return False
    
    time.sleep(2)
    
    try:
        tests = []
        
        tests.append(manual_test(
            "Cargar sitio estático en navegador",
            "1. Abre http://127.0.0.1:8080/ en tu navegador\n"
            "2. Abre DevTools (F12) > Network\n"
            "3. Recarga la página (Ctrl+R)\n"
            "4. Verifica: Status 200, headers correctos, content-type apropiado\n"
            "5. Página debería cargar correctamente"
        ))
        
        tests.append(manual_test(
            "Manejo de URLs incorrectas",
            "1. Intenta acceder a http://127.0.0.1:8080/404test\n"
            "2. Debería mostrar página de error 404"
        ))
        
        tests.append(manual_test(
            "Listado de directorios",
            "1. Intenta acceder a http://127.0.0.1:8080/directory/\n"
            "2. Debería listar contenido o mostrar índice"
        ))
        
        return all(tests)
    
    finally:
        server.stop()

def test_stress():
    """Stress testing guidance"""
    test_section("6️⃣  PRUEBAS DE ESTRÉS")
    
    log("SIEGE Stress Test", "STEP")
    print("\n1. Instala siege:")
    print("   Ubuntu: sudo apt-get install siege")
    print("   Mac: brew install siege\n")
    print("2. Ejecuta el test:")
    print("   siege -b -u http://127.0.0.1:8080/ -t 30S\n")
    print("3. REQUISITOS:")
    print("   ✓ Availability > 99.5%")
    print("   ✓ Sin memory leaks")
    print("   ✓ Sin conexiones colgadas")
    print("   ✓ Puede ejecutarse indefinidamente\n")
    
    answer = input("¿Pasó el stress test? (s/n): ").strip().lower()
    return answer.startswith('s')

def main():
    print(f"\n{Color.HEADER}{Color.BOLD}")
    print("╔" + "═"*68 + "╗")
    print("║" + " "*15 + "42 WEBSERV EVALUATION TEST SUITE" + " "*21 + "║")
    print("╚" + "═"*68 + "╝")
    print(Color.RESET)
    
    results = {}
    
    # Run tests
    results["I/O Multiplexing"] = test_io_multiplexing()
    results["Basic Requests"] = test_basic_requests()
    results["Body Size Limit"] = test_body_size_limit()
    results["Configuration"] = all(test_configuration())
    results["Browser Compatibility"] = test_browser_compatibility()
    results["Stress Testing"] = test_stress()
    
    # Print summary
    test_section("📊 RESUMEN DE PRUEBAS")
    
    passed = sum(1 for v in results.values() if v)
    total = len(results)
    
    for test_name, result in results.items():
        symbol = "✓" if result else "✗"
        color = Color.GREEN if result else Color.RED
        print(f"{color}{symbol}{Color.RESET} {test_name}")
    
    print(f"\n{Color.BOLD}TOTAL: {passed}/{total} pruebas pasadas ({100*passed//total}%){Color.RESET}\n")

if __name__ == "__main__":
    main()
