#!/usr/bin/env python3
"""
Webserv Test Suite - Evaluación completa del servidor web
Basado en los requisitos de 42Webserv
"""

import subprocess
import time
import socket
import sys
import os
import signal
import requests
from pathlib import Path
from typing import Tuple, Optional
import threading
import json

# Colors for output
class Color:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    RESET = '\033[0m'
    BOLD = '\033[1m'

class WebservTester:
    def __init__(self, binary_path: str = "./webserv", config_path: str = "./configs/default.conf"):
        self.binary_path = binary_path
        self.config_path = config_path
        self.server_process = None
        self.test_results = []
        
    def log(self, message: str, level: str = "INFO"):
        """Logging with colors"""
        levels = {
            "INFO": Color.BLUE,
            "SUCCESS": Color.GREEN,
            "ERROR": Color.RED,
            "WARNING": Color.YELLOW,
            "MANUAL": Color.CYAN,
            "HEADER": Color.HEADER + Color.BOLD,
        }
        color = levels.get(level, Color.RESET)
        print(f"{color}[{level}]{Color.RESET} {message}")
    
    def start_server(self, config: str = None, port: int = 8080) -> bool:
        """Start webserv server"""
        config_file = config or self.config_path
        cmd = [self.binary_path, config_file]
        
        try:
            self.server_process = subprocess.Popen(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                preexec_fn=os.setsid
            )
            time.sleep(1)  # Wait for server to start
            
            # Check if process is still alive
            if self.server_process.poll() is not None:
                self.log("Server failed to start", "ERROR")
                return False
            
            self.log(f"Server started on PID {self.server_process.pid}", "SUCCESS")
            return True
        except Exception as e:
            self.log(f"Failed to start server: {e}", "ERROR")
            return False
    
    def stop_server(self):
        """Stop webserv server"""
        if self.server_process:
            try:
                os.killpg(os.getpgid(self.server_process.pid), signal.SIGTERM)
                self.server_process.wait(timeout=5)
                self.log("Server stopped", "SUCCESS")
            except subprocess.TimeoutExpired:
                os.killpg(os.getpgid(self.server_process.pid), signal.SIGKILL)
                self.log("Server killed", "WARNING")
            except Exception as e:
                self.log(f"Error stopping server: {e}", "ERROR")
    
    def is_port_open(self, host: str = "127.0.0.1", port: int = 8080) -> bool:
        """Check if port is open"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(1)
            result = sock.connect_ex((host, port))
            sock.close()
            return result == 0
        except:
            return False
    
    def test_http_request(self, method: str, url: str, data: str = None, 
                         headers: dict = None, expected_status: int = None) -> Tuple[bool, dict]:
        """Send HTTP request and check response"""
        try:
            if headers is None:
                headers = {}
            
            if method.upper() == "GET":
                response = requests.get(url, headers=headers, timeout=5)
            elif method.upper() == "POST":
                response = requests.post(url, data=data, headers=headers, timeout=5)
            elif method.upper() == "DELETE":
                response = requests.delete(url, headers=headers, timeout=5)
            else:
                return False, {"error": "Unsupported method"}
            
            result = {
                "status_code": response.status_code,
                "headers": dict(response.headers),
                "content": response.text[:500],  # First 500 chars
            }
            
            passed = True
            if expected_status and response.status_code != expected_status:
                passed = False
            
            return passed, result
        except Exception as e:
            return False, {"error": str(e)}
    
    def record_test(self, name: str, passed: bool, details: str = ""):
        """Record test result"""
        self.test_results.append({
            "name": name,
            "passed": passed,
            "details": details
        })
    
    # ==================== MANDATORY TESTS ====================
    
    def test_code_review(self):
        """Manual code review checklist"""
        self.log("\n" + "="*60, "HEADER")
        self.log("MANDATORY: CODE REVIEW CHECKLIST", "HEADER")
        self.log("="*60 + "\n", "HEADER")
        
        questions = [
            ("I/O Multiplexing", "¿Qué función de multiplexing de I/O se usa? (select, poll, epoll, kqueue)"),
            ("Select Usage", "¿Cómo funciona select? ¿Se evalúan READ y WRITE al mismo tiempo?"),
            ("Main Loop", "¿Hay un único select/poll en el main loop que gestiona clients y accepts?"),
            ("Single R/W per client", "¿Hay solo UN read o UN write por client por select?"),
            ("Error Handling", "Verificar: ¿Todos los read/recv/write/send manejan errores correctamente?"),
            ("Errno Check", "¿Se chequea errno después de read/recv/write/send? (DEBE SER NO)"),
            ("No Direct I/O", "¿Hay lectura/escritura directa de file descriptors sin select? (DEBE SER NO)"),
        ]
        
        for question_id, question_text in questions:
            self.log(f"\n👉 {question_id}: {question_text}", "MANUAL")
            answer = input(f"   Respuesta (sí/no/detalles): ").strip()
            self.record_test(f"Code Review: {question_id}", len(answer) > 0, answer)
    
    def test_compilation(self) -> bool:
        """Test compilation"""
        self.log("\n" + "="*60, "HEADER")
        self.log("MANDATORY: COMPILATION TEST", "HEADER")
        self.log("="*60 + "\n", "HEADER")
        
        try:
            result = subprocess.run(
                ["make", "clean"],
                cwd=os.path.dirname(self.binary_path) or ".",
                capture_output=True,
                timeout=10
            )
            
            result = subprocess.run(
                ["make"],
                cwd=os.path.dirname(self.binary_path) or ".",
                capture_output=True,
                timeout=30
            )
            
            if result.returncode == 0:
                self.log("✓ Compilation successful", "SUCCESS")
                self.record_test("Compilation", True)
                return True
            else:
                self.log(f"✗ Compilation failed:\n{result.stderr.decode()}", "ERROR")
                self.record_test("Compilation", False, result.stderr.decode())
                return False
        except Exception as e:
            self.log(f"✗ Compilation error: {e}", "ERROR")
            self.record_test("Compilation", False, str(e))
            return False
    
    def test_basic_requests(self):
        """Test basic HTTP requests"""
        self.log("\n" + "="*60, "HEADER")
        self.log("MANDATORY: BASIC HTTP REQUESTS", "HEADER")
        self.log("="*60 + "\n", "HEADER")
        
        if not self.start_server():
            self.log("Cannot start server", "ERROR")
            return
        
        time.sleep(2)
        
        tests = [
            ("GET /", "GET", "http://127.0.0.1:8080/", None, 200),
            ("GET /index.html", "GET", "http://127.0.0.1:8080/index.html", None, 200),
            ("POST /post_body", "POST", "http://127.0.0.1:8080/post_body", "test data", 200),
            ("DELETE (if allowed)", "DELETE", "http://127.0.0.1:8080/", None, None),
            ("GET /nonexistent", "GET", "http://127.0.0.1:8080/nonexistent", None, 404),
        ]
        
        for test_name, method, url, data, expected_status in tests:
            passed, result = self.test_http_request(method, url, data, expected_status=expected_status)
            status = "✓" if passed else "✗"
            self.log(f"{status} {test_name} - Status: {result.get('status_code', 'ERROR')}", 
                    "SUCCESS" if passed else "ERROR")
            self.record_test(test_name, passed, str(result))
        
        self.stop_server()
    
    def test_body_limit(self):
        """Test client body size limit"""
        self.log("\n" + "="*60, "HEADER")
        self.log("MANDATORY: CLIENT BODY SIZE LIMIT", "HEADER")
        self.log("="*60 + "\n", "HEADER")
        
        if not self.start_server():
            return
        
        time.sleep(1)
        
        # Small body (should pass)
        small_body = "x" * 50
        passed, result = self.test_http_request(
            "POST", 
            "http://127.0.0.1:8080/post_body",
            small_body,
            expected_status=200
        )
        self.log(f"{'✓' if passed else '✗'} Small body (50 bytes): {result.get('status_code', 'ERROR')}", 
                "SUCCESS" if passed else "ERROR")
        self.record_test("Body Limit: Small Body", passed)
        
        # Large body (should fail)
        large_body = "x" * 500
        passed, result = self.test_http_request(
            "POST",
            "http://127.0.0.1:8080/post_body",
            large_body,
            expected_status=413  # Payload Too Large
        )
        self.log(f"{'✓' if passed else '✗'} Large body (500 bytes): {result.get('status_code', 'ERROR')}", 
                "SUCCESS" if passed else "ERROR")
        self.record_test("Body Limit: Large Body", passed)
        
        self.stop_server()
    
    def test_get_post_delete(self):
        """Test GET, POST, DELETE methods"""
        self.log("\n" + "="*60, "HEADER")
        self.log("MANDATORY: HTTP METHODS (GET/POST/DELETE)", "HEADER")
        self.log("="*60 + "\n", "HEADER")
        
        if not self.start_server():
            return
        
        time.sleep(1)
        
        # GET test
        passed, result = self.test_http_request("GET", "http://127.0.0.1:8080/", expected_status=200)
        self.log(f"{'✓' if passed else '✗'} GET request: {result.get('status_code', 'ERROR')}", 
                "SUCCESS" if passed else "ERROR")
        self.record_test("HTTP Methods: GET", passed)
        
        # POST test
        passed, result = self.test_http_request("POST", "http://127.0.0.1:8080/post_body", 
                                               "test data", expected_status=200)
        self.log(f"{'✓' if passed else '✗'} POST request: {result.get('status_code', 'ERROR')}", 
                "SUCCESS" if passed else "ERROR")
        self.record_test("HTTP Methods: POST", passed)
        
        # DELETE test
        passed, result = self.test_http_request("DELETE", "http://127.0.0.1:8080/", expected_status=405)
        self.log(f"{'✓' if passed else '✗'} DELETE (should be forbidden): {result.get('status_code', 'ERROR')}", 
                "SUCCESS" if (result.get('status_code') == 405) else "ERROR")
        self.record_test("HTTP Methods: DELETE (forbidden)", result.get('status_code') == 405)
        
        self.stop_server()
    
    def test_unknown_method(self):
        """Test unknown HTTP method (should not crash)"""
        self.log("\n" + "="*60, "HEADER")
        self.log("MANDATORY: UNKNOWN METHOD HANDLING", "HEADER")
        self.log("="*60 + "\n", "HEADER")
        
        if not self.start_server():
            return
        
        time.sleep(1)
        
        try:
            # Send raw HTTP request with UNKNOWN method
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.connect(("127.0.0.1", 8080))
            sock.sendall(b"UNKNOWN / HTTP/1.1\r\nHost: localhost\r\n\r\n")
            response = sock.recv(1024)
            sock.close()
            
            if response:
                self.log(f"✓ Server handled unknown method without crashing", "SUCCESS")
                self.record_test("Unknown Method Handling", True)
            else:
                self.log(f"✗ No response to unknown method", "ERROR")
                self.record_test("Unknown Method Handling", False)
        except Exception as e:
            self.log(f"✗ Server crashed with unknown method: {e}", "ERROR")
            self.record_test("Unknown Method Handling", False, str(e))
        
        self.stop_server()
    
    # ==================== CONFIGURATION TESTS ====================
    
    def test_configuration(self):
        """Manual configuration tests"""
        self.log("\n" + "="*60, "HEADER")
        self.log("MANDATORY: CONFIGURATION TESTS", "HEADER")
        self.log("="*60 + "\n", "HEADER")
        
        config_tests = [
            ("Multiple servers", "¿Se pueden configurar múltiples servidores con diferentes puertos?"),
            ("Multiple hostnames", "¿Se pueden configurar múltiples hostnames? (curl --resolve)"),
            ("Default error page", "¿Se puede cambiar la página de error 404?"),
            ("Body limit", "¿Funciona el límite de tamaño de cliente (client_max_body_size)?"),
            ("Routes", "¿Se pueden configurar rutas a diferentes directorios?"),
            ("Default file", "¿Se busca un archivo por defecto en directorios? (index)"),
            ("Allowed methods", "¿Se pueden restriccionar métodos HTTP por ruta?"),
        ]
        
        for test_id, question_text in config_tests:
            self.log(f"\n👉 {test_id}: {question_text}", "MANUAL")
            answer = input(f"   ¿Funciona? (sí/no/detalles): ").strip()
            self.record_test(f"Config: {test_id}", answer.lower() == "sí" or "yes" in answer.lower())
    
    # ==================== BROWSER TESTS ====================
    
    def test_browser(self):
        """Manual browser tests"""
        self.log("\n" + "="*60, "HEADER")
        self.log("MANDATORY: BROWSER TESTS (MANUAL)", "HEADER")
        self.log("="*60 + "\n", "HEADER")
        
        if not self.start_server():
            return
        
        time.sleep(2)
        
        self.log("\n📱 BROWSER TEST CHECKLIST:", "MANUAL")
        self.log("1. Abre http://127.0.0.1:8080/ en tu navegador", "MANUAL")
        self.log("2. Abre DevTools (F12) y ve a la pestaña Network", "MANUAL")
        self.log("3. Recarga la página y verifica:", "MANUAL")
        print("   - Status code 200")
        print("   - Content-Type correcto")
        print("   - Headers Request y Response correctos")
        print("4. Intenta con URLs incorrectas (404)")
        print("5. Intenta listar un directorio")
        print("6. Intenta redirects si están configurados")
        
        answer = input("\n¿Todo funciona correctamente en el navegador? (sí/no): ").strip()
        self.record_test("Browser: Static Website", answer.lower() == "sí")
        
        self.stop_server()
    
    # ==================== STRESS TESTS ====================
    
    def test_stress_manual(self):
        """Manual stress test with siege guidance"""
        self.log("\n" + "="*60, "HEADER")
        self.log("STRESS TEST: SIEGE (MANUAL)", "HEADER")
        self.log("="*60 + "\n", "HEADER")
        
        if not self.start_server():
            return
        
        time.sleep(2)
        
        self.log("\n🔥 SIEGE STRESS TEST INSTRUCTIONS:", "MANUAL")
        self.log("1. Instala siege:", "MANUAL")
        print("   Ubuntu: sudo apt-get install siege")
        print("   Mac: brew install siege")
        self.log("\n2. Ejecuta:", "MANUAL")
        print("   siege -b -u http://127.0.0.1:8080/ -t 30S")
        self.log("\n3. Verifica:", "MANUAL")
        print("   - Availability > 99.5%")
        print("   - No memory leaks (monitora el proceso)")
        print("   - Sin conexiones colgadas")
        
        answer = input("\n¿Pasó el stress test? (sí/no): ").strip()
        self.record_test("Stress Test: Siege", answer.lower() == "sí")
        
        self.stop_server()
    
    # ==================== PORT TESTS ====================
    
    def test_port_configuration(self):
        """Test port configuration"""
        self.log("\n" + "="*60, "HEADER")
        self.log("MANDATORY: PORT CONFIGURATION TESTS", "HEADER")
        self.log("="*60 + "\n", "HEADER")
        
        self.log("\n👉 Prueba 1: Múltiples puertos", "MANUAL")
        self.log("Configura diferentes puertos (8080, 8081, 8082) con diferentes sitios web", "MANUAL")
        self.log("Abre en navegador cada puerto y verifica que muestra el sitio correcto", "MANUAL")
        answer = input("¿Funciona? (sí/no): ").strip()
        self.record_test("Port Config: Multiple Ports", answer.lower() == "sí")
        
        self.log("\n👉 Prueba 2: Puerto duplicado", "MANUAL")
        self.log("Intenta configurar el mismo puerto múltiples veces", "MANUAL")
        self.log("El servidor NO debe iniciar (error esperado)", "MANUAL")
        answer = input("¿Rechazó el puerto duplicado? (sí/no): ").strip()
        self.record_test("Port Config: Duplicate Port Rejection", answer.lower() == "sí")
    
    # ==================== REPORT GENERATION ====================
    
    def print_report(self):
        """Print test report"""
        self.log("\n" + "="*80, "HEADER")
        self.log("TEST REPORT SUMMARY", "HEADER")
        self.log("="*80 + "\n", "HEADER")
        
        passed = sum(1 for t in self.test_results if t["passed"])
        total = len(self.test_results)
        
        for test in self.test_results:
            status = "✓ PASS" if test["passed"] else "✗ FAIL"
            color = "SUCCESS" if test["passed"] else "ERROR"
            self.log(f"{status} - {test['name']}", color)
            if test["details"] and len(test["details"]) < 100:
                print(f"        {test['details']}\n")
        
        self.log("\n" + "="*80, "HEADER")
        self.log(f"TOTAL: {passed}/{total} tests passed ({100*passed//total}%)", "HEADER")
        self.log("="*80 + "\n", "HEADER")
        
        # Save report
        report_path = "test_report.json"
        with open(report_path, "w") as f:
            json.dump(self.test_results, f, indent=2)
        self.log(f"Report saved to {report_path}", "SUCCESS")
    
    def run_all_tests(self):
        """Run all tests"""
        self.log("\n" + "="*80, "HEADER")
        self.log("🚀 WEBSERV COMPLETE TEST SUITE 🚀", "HEADER")
        self.log("="*80 + "\n", "HEADER")
        
        # Compilation
        if not self.test_compilation():
            self.log("Compilation failed, aborting", "ERROR")
            return
        
        # Mandatory tests
        self.test_code_review()
        self.test_basic_requests()
        self.test_body_limit()
        self.test_get_post_delete()
        self.test_unknown_method()
        
        # Configuration tests
        self.test_configuration()
        
        # Browser tests
        self.test_browser()
        
        # Port tests
        self.test_port_configuration()
        
        # Stress tests
        self.test_stress_manual()
        
        # Report
        self.print_report()


def main():
    if len(sys.argv) > 1 and sys.argv[1] == "--quick":
        # Quick mode: only compilation and basic tests
        tester = WebservTester()
        tester.test_compilation()
        tester.test_basic_requests()
        tester.print_report()
    else:
        # Full test suite
        tester = WebservTester()
        tester.run_all_tests()


if __name__ == "__main__":
    main()
