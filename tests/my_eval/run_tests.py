#!/usr/bin/env python3
"""
Webserv Test Suite - Punto de entrada único
Ejecuta tests automatizados y guía pruebas manuales según requisitos 42Webserv
"""

import subprocess
import socket
import time
import sys
import os
import signal
import requests
import json
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

class WebservTester:
    def __init__(self, binary="./webserv", config="./configs/default.conf"):
        self.binary = binary
        self.config = config
        self.process = None
        self.results = []
    
    def log(self, msg, level="INFO"):
        levels = {
            "INFO": Color.BLUE,
            "OK": Color.GREEN,
            "FAIL": Color.RED,
            "TODO": Color.YELLOW,
            "STEP": Color.CYAN,
        }
        color = levels.get(level, Color.RESET)
        print(f"{color}[{level}]{Color.RESET} {msg}")
    
    def header(self, title):
        print(f"\n{Color.HEADER}{'='*70}{Color.RESET}")
        print(f"{Color.HEADER}  {title}{Color.RESET}")
        print(f"{Color.HEADER}{'='*70}{Color.RESET}\n")
    
    def start_server(self):
        try:
            self.process = subprocess.Popen(
                [self.binary, self.config],
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                preexec_fn=os.setsid
            )
            time.sleep(1)
            if self.process.poll() is None:
                self.log(f"Server started (PID {self.process.pid})", "OK")
                return True
            return False
        except Exception as e:
            self.log(f"Failed to start: {e}", "FAIL")
            return False
    
    def stop_server(self):
        if self.process:
            try:
                os.killpg(os.getpgid(self.process.pid), signal.SIGTERM)
                self.process.wait(timeout=3)
                self.log("Server stopped", "OK")
            except:
                try:
                    os.killpg(os.getpgid(self.process.pid), signal.SIGKILL)
                except:
                    pass
    
    def record(self, name, passed, details=""):
        self.results.append({"name": name, "passed": passed, "details": details})
    
    def socket_request(self, method, path, body=None):
        """Raw HTTP request via socket"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            sock.connect(("127.0.0.1", 8080))
            
            request = f"{method} {path} HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n"
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
    
    # ==================== TESTS ====================
    
    def test_compilation(self):
        self.header("COMPILACIÓN")
        try:
            result = subprocess.run(
                ["make", "clean"],
                cwd=".",
                capture_output=True,
                timeout=10
            )
            result = subprocess.run(
                ["make"],
                cwd=".",
                capture_output=True,
                timeout=30
            )
            if result.returncode == 0:
                self.log("✓ Compilación exitosa", "OK")
                self.record("Compilation", True)
                return True
            else:
                self.log(f"✗ Error: {result.stderr.decode()[:200]}", "FAIL")
                self.record("Compilation", False)
                return False
        except Exception as e:
            self.log(f"✗ {e}", "FAIL")
            self.record("Compilation", False)
            return False
    
    def test_basic_http(self):
        self.header("PETICIONES HTTP BÁSICAS")
        if not self.start_server():
            return False
        
        time.sleep(1)
        tests_passed = 0
        
        try:
            # GET
            resp = self.socket_request("GET", "/")
            if "200" in resp:
                self.log("✓ GET / → 200", "OK")
                tests_passed += 1
            else:
                self.log("✗ GET / failed", "FAIL")
            self.record("GET /", "200" in resp)
            
            # POST
            resp = self.socket_request("POST", "/post_body", "test")
            if "200" in resp:
                self.log("✓ POST /post_body → 200", "OK")
                tests_passed += 1
            else:
                self.log("✗ POST /post_body failed", "FAIL")
            self.record("POST /post_body", "200" in resp)
            
            # DELETE (should fail)
            resp = self.socket_request("DELETE", "/")
            if "405" in resp or "403" in resp:
                self.log("✓ DELETE / → 405 (forbidden)", "OK")
                tests_passed += 1
            else:
                self.log("✗ DELETE should be forbidden", "FAIL")
            self.record("DELETE forbidden", "405" in resp or "403" in resp)
            
            # Unknown method (no crash)
            resp = self.socket_request("BADMETHOD", "/")
            if resp and "ERROR" not in resp:
                self.log("✓ Unknown method handled", "OK")
                tests_passed += 1
            else:
                self.log("✗ Server might have crashed", "FAIL")
            self.record("Unknown method", resp and "ERROR" not in resp)
            
            # 404
            resp = self.socket_request("GET", "/nonexistent12345")
            if "404" in resp:
                self.log("✓ GET /nonexistent → 404", "OK")
                tests_passed += 1
            else:
                self.log("✗ 404 not returned", "FAIL")
            self.record("404 error", "404" in resp)
        
        finally:
            self.stop_server()
        
        return tests_passed >= 4
    
    def test_body_limit(self):
        self.header("LÍMITE DE BODY SIZE")
        if not self.start_server():
            return False
        
        time.sleep(1)
        
        try:
            # Small (should pass)
            small = "x" * 50
            resp = self.socket_request("POST", "/post_body", small)
            small_ok = "200" in resp
            self.log(f"{'✓' if small_ok else '✗'} Small body (50 bytes) → {'200' if small_ok else 'FAIL'}", 
                    "OK" if small_ok else "FAIL")
            self.record("Body limit: Small", small_ok)
            
            # Large (should fail with 413)
            large = "x" * 500
            resp = self.socket_request("POST", "/post_body", large)
            large_ok = "413" in resp or "400" in resp
            self.log(f"{'✓' if large_ok else '✗'} Large body (500 bytes) → {'413' if large_ok else 'FAIL'}", 
                    "OK" if large_ok else "FAIL")
            self.record("Body limit: Large", large_ok)
        
        finally:
            self.stop_server()
        
        return small_ok and large_ok
    
    def code_review(self):
        self.header("CODE REVIEW - I/O MULTIPLEXING")
        self.log("Responde estas preguntas sobre tu código:", "STEP")
        
        questions = [
            "¿Qué mecanismo usas? (select/poll/epoll/kqueue)",
            "¿READ y WRITE al MISMO TIEMPO? (sí/no)",
            "¿UN ÚNICO select en main loop? (sí/no)",
            "¿1 read/write por cliente por ciclo? (sí/no)",
            "¿Se chequea errno después de read/write? DEBE SER NO (sí/no)",
        ]
        
        answers = []
        for i, q in enumerate(questions, 1):
            print(f"\n{i}. {q}")
            ans = input("   → ").strip().lower()
            answers.append(ans)
        
        # Validar respuesta 5 (errno check)
        if answers[4] == "no":
            self.log("✓ Correcto - no se chequea errno", "OK")
            self.record("Code Review: errno", True)
        else:
            self.log("✗ DEBE SER NO - se chequea errno está prohibido", "FAIL")
            self.record("Code Review: errno", False)
        
        print("\n" + "="*70)
        print("Respuestas guardadas para revisión manual")
        return True
    
    def manual_tests(self):
        self.header("PRUEBAS MANUALES")
        
        tests = [
            ("Múltiples puertos", "¿Puedes configurar diferentes puertos (8080, 8081)?"),
            ("Métodos restringidos", "¿Se puede restringir métodos por ruta (GET solo en /)?"),
            ("Navegador", "¿Carga el sitio en el navegador correctamente?"),
            ("Stress test", "¿Passó siege con availability > 99.5%?"),
        ]
        
        for test_name, question in tests:
            print(f"\n👉 {test_name}")
            print(f"   {question}")
            ans = input("   ¿Pasó? (s/n): ").strip().lower()
            self.record(f"Manual: {test_name}", ans.startswith('s'))
    
    def print_report(self):
        self.header("REPORTE FINAL")
        
        passed = sum(1 for r in self.results if r["passed"])
        total = len(self.results)
        
        for r in self.results:
            status = "✓" if r["passed"] else "✗"
            color = "GREEN" if r["passed"] else "RED"
            symbol = "✓" if r["passed"] else "✗"
            print(f"{Color.GREEN if r['passed'] else Color.RED}{symbol}{Color.RESET} {r['name']}")
        
        print(f"\n{Color.BOLD}TOTAL: {passed}/{total} ({100*passed//total}%){Color.RESET}\n")
        
        if total > 0:
            with open("test_report.json", "w") as f:
                json.dump(self.results, f, indent=2)
            self.log(f"Report guardado en: test_report.json", "OK")
    
    def run(self, mode="full"):
        print(f"\n{Color.HEADER}{Color.BOLD}")
        print("╔" + "═"*68 + "╗")
        print("║" + " "*20 + "WEBSERV TEST SUITE" + " "*30 + "║")
        print("╚" + "═"*68 + "╝")
        print(Color.RESET + "\n")
        
        # Compilación
        if not self.test_compilation():
            self.log("Compilación falló - abortando", "FAIL")
            return
        
        # Tests básicos
        self.test_basic_http()
        self.test_body_limit()
        
        # Code review
        self.code_review()
        
        # Manual tests
        self.manual_tests()
        
        # Reporte
        self.print_report()


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Webserv Test Suite')
    parser.add_argument('-m', '--mode', choices=['quick', 'full'], default='full',
                       help='quick: solo tests automatizados, full: + manual')
    parser.add_argument('-b', '--binary', default='./webserv', help='Path to webserv binary')
    parser.add_argument('-c', '--config', default='./configs/default.conf', help='Config file')
    
    args = parser.parse_args()
    
    tester = WebservTester(args.binary, args.config)
    tester.run(args.mode)


if __name__ == "__main__":
    main()
