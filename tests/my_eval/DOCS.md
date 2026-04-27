# 📊 Webserv Test Suite - Documentación Completa

## 🚀 Quick Start

```bash
cd /home/macastro/workspace/github/webserv

# Ejecutar tests (punto de entrada único)
python3 tests/run_tests.py

# Solo tests automatizados (5 min)
python3 tests/run_tests.py --mode quick

# O con make
make -C tests quick-test
```

---

## 📁 Estructura de Tests

```
tests/
├── run_tests.py              👈 PUNTO DE ENTRADA ÚNICO
├── quick_tests.sh            Tests automatizados (bash/curl)
├── eval_42.py                Evaluación enfocada (Python)
├── test_suite.py             Suite completa (Python)
├── setup_test_configs.sh     Generador de configs
├── Makefile                  Automatización
└── DOCS.md                   Este archivo
```

---

## 🎯 Qué hace cada test

### `run_tests.py` (RECOMENDADO)
**Punto de entrada único** - Combina tests automatizados + guía interactiva

```bash
python3 tests/run_tests.py
```

**Incluye:**
- ✓ Compilación
- ✓ GET/POST/DELETE requests
- ✓ Unknown methods
- ✓ 404 errors
- ✓ Body size limit
- ✓ Code review interactivo (I/O Multiplexing)
- ✓ Pruebas manuales guiadas

**Output:** Console + test_report.json

---

### `quick_tests.sh` (AUTOMATIZADO)
Tests rápidos solo bash/curl - Sin dependencias Python

```bash
./tests/quick_tests.sh
```

**Verifica:**
- Compilación
- GET/POST/DELETE
- 404, headers
- Body limits

**Duración:** ~5 minutos

---

### `eval_42.py` (EVALUACIÓN 42)
Pruebas interactivas enfocadas en requisitos de evaluación

```bash
python3 tests/eval_42.py
```

**Incluye:**
- Code review paso a paso
- HTTP requests
- Body limits
- Configuration
- Browser tests (manual)
- Stress testing

**Duración:** ~30 minutos

---

### `test_suite.py` (COMPLETA)
Suite exhaustiva con todos los tests

```bash
python3 tests/test_suite.py --quick
```

**Duración:** 5 min (--quick) o 45 min (completa)

---

## 📋 Requisitos a Validar

### ✅ Obligatorios

| Criterio | Test | Status |
|----------|------|--------|
| Compilación sin re-link | Automatizado | ✓ |
| GET requests | Automatizado | ✓ |
| POST requests | Automatizado | ✓ |
| DELETE (forbidden) | Automatizado | ✓ |
| Unknown methods (no crash) | Automatizado | ✓ |
| 404 errors | Automatizado | ✓ |
| Body size limit | Automatizado | ✓ |
| I/O Multiplexing | Code review | Manual |
| Múltiples puertos | Configuración | Manual |
| Métodos restringidos | Configuración | Manual |
| Navegador | Browser | Manual |
| Stress test (siege) | Manual | Manual |

### 📌 Críticos - NO Fallar

1. **Compilación**: `make clean && make` sin errores
2. **I/O Multiplexing**:
   - UN ÚNICO select en main loop
   - READ y WRITE al MISMO TIEMPO
   - 1 lectura/escritura MAX por cliente por ciclo
   - NO se chequea errno
   - TODO I/O a través de select

3. **Stress Test**: Availability > 99.5%

---

## 🔧 Comandos Útiles

### Tests
```bash
# Punto de entrada
python3 tests/run_tests.py

# Solo automatizados (rápido)
./tests/quick_tests.sh

# Evaluación 42
python3 tests/eval_42.py

# Suite completa
python3 tests/test_suite.py

# Usando make
make -C tests help
make -C tests quick-test
make -C tests eval-test
```

### Manual Testing
```bash
# GET
curl http://127.0.0.1:8080/

# POST
curl -X POST -d "test data" http://127.0.0.1:8080/post_body

# DELETE (debe fallar)
curl -X DELETE http://127.0.0.1:8080/

# Con verbose
curl -v http://127.0.0.1:8080/

# Ver solo headers
curl -i http://127.0.0.1:8080/
```

### Stress Test
```bash
# Instalar siege (una sola vez)
sudo apt-get install siege

# Ejecutar
siege -b -u http://127.0.0.1:8080/ -t 60S
```

### Debugging
```bash
# Matar procesos previos
pkill -f webserv

# Monitor de memoria
watch -n 1 'ps aux | grep webserv'

# Ver qué usa el puerto
sudo lsof -i :8080

# Compilación limpia
make clean && make
```

---

## 📊 Flujo Recomendado

### 1. Desarrollo (Daily)
```bash
make clean && make              # Compilar
./tests/quick_tests.sh          # Test rápido
# Si falla → Fix → Repeat
# Si pasa → Continuar
```

### 2. Pre-Evaluación
```bash
python3 tests/run_tests.py      # Tests + Code review
# Revisar requisitos en DOCS.md
# Pruebas manuales en navegador
# Stress test con siege
```

### 3. Limpiar
```bash
make -C tests clean             # Limpiar logs
pkill -f webserv                # Matar procesos
```

---

## 🎓 Requisitos Detallados

### I/O Multiplexing ⚠️ CRÍTICO

**Qué se evalúa:**
- Mecanismo: select/poll/epoll/kqueue
- UN ÚNICO select en main loop
- READ y WRITE evaluados al MISMO TIEMPO
- 1 lectura/escritura MAX por cliente por ciclo
- Manejo correcto de errores
- NO se chequea errno
- TODO I/O a través de select

**Verificación:**
```cpp
// ✓ CORRECTO
FD_SET(fd, &read_set);
FD_SET(fd, &write_set);
select(maxfd+1, &read_set, &write_set, NULL, NULL);

// ✗ INCORRECTO
select(maxfd+1, &read_set, NULL, NULL, NULL);  // Solo read
```

### HTTP Requests

**GET**
```bash
curl http://127.0.0.1:8080/     # Esperado: 200
```

**POST**
```bash
curl -X POST -d "data" http://127.0.0.1:8080/post_body   # Esperado: 200
```

**DELETE (debe rechazarse)**
```bash
curl -X DELETE http://127.0.0.1:8080/   # Esperado: 405
```

**Unknown Method (no crash)**
```bash
curl -X BADMETHOD http://127.0.0.1:8080/   # Esperado: No crash
```

**404 Error**
```bash
curl http://127.0.0.1:8080/nonexistent     # Esperado: 404
```

### Body Size Limit

**Configuración:**
```conf
location /post_body {
    allow_methods POST;
    client_max_body_size 100;
}
```

**Test:**
```bash
# Pequeño (50 bytes) → 200
curl -X POST --data "$(printf 'x%.0s' {1..50})" http://127.0.0.1:8080/post_body

# Grande (500 bytes) → 413
curl -X POST --data "$(printf 'x%.0s' {1..500})" http://127.0.0.1:8080/post_body
```

### Configuración

#### Múltiples Puertos
```conf
server { listen 8080; server_name site1; root docs/site1; }
server { listen 8081; server_name site2; root docs/site2; }
```

**Test:**
```bash
curl http://127.0.0.1:8080/
curl http://127.0.0.1:8081/
```

#### Métodos Restringidos
```conf
location / {
    allow_methods GET;
}

location /post_body {
    allow_methods POST;
}
```

**Test:**
```bash
curl http://127.0.0.1:8080/                    # Funciona
curl -X POST http://127.0.0.1:8080/            # Rechazado (405)
curl -X POST http://127.0.0.1:8080/post_body   # Funciona
```

#### Páginas de Error
```conf
error_page 404 /error_pages/404.html;
```

**Test:**
```bash
curl http://127.0.0.1:8080/nonexistent
```

### Navegador

**Qué revisar:**
1. Abre http://127.0.0.1:8080/
2. F12 → Network tab
3. Recarga (Ctrl+R)
4. Verifica:
   - Status code 200
   - Content-Type correcto
   - Headers OK
5. Prueba URLs incorrectas
6. Intenta listar directorios

### Stress Test

**Requisitos:**
- Availability > 99.5%
- Sin memory leaks
- Sin conexiones colgadas
- Funciona indefinidamente

**Instalación:**
```bash
sudo apt-get install siege   # Ubuntu
brew install siege           # Mac
```

**Ejecución:**
```bash
siege -b -u http://127.0.0.1:8080/ -t 60S
```

**Monitoreo simultáneo (3 terminales):**
```bash
# Terminal 1
./webserv configs/default.conf

# Terminal 2
siege -b -u http://127.0.0.1:8080/ -t 60S

# Terminal 3
watch -n 1 'ps aux | grep webserv'
```

---

## 🐛 Troubleshooting

| Problema | Solución |
|----------|----------|
| Port 8080 in use | `pkill -f webserv` |
| Connection refused | Verifica que webserv está corriendo |
| Compilation error | `make clean && make` |
| Segfault | Revisar: ¿Hay I/O sin select? |
| Memory leak | `watch 'ps aux \| grep webserv'` |
| Tests fallan | Revisar: Status codes, headers |

---

## ✅ Checklist Pre-Evaluación

- [ ] Compilación: `make clean && make` ✓
- [ ] Quick tests: `./tests/quick_tests.sh` → ALL PASS
- [ ] Run tests: `python3 tests/run_tests.py` → ALL PASS
- [ ] Code review: I/O Multiplexing correcto ✓
- [ ] Navegador: Sitio carga correctamente ✓
- [ ] Stress: `siege -b -u http://127.0.0.1:8080/ -t 60S` → >99.5%
- [ ] Memory: No crece indefinidamente ✓
- [ ] Múltiples puertos: Funcionan ✓
- [ ] Métodos restrictos: Funcionan ✓
- [ ] Error pages: Funcionan ✓

---

## 🚀 Próximos Pasos

1. **Ahora:**
```bash
cd /home/macastro/workspace/github/webserv
python3 tests/run_tests.py
```

2. **Si falla algo:** Revisa la sección de requisitos arriba

3. **Si todo pasa:** Listo para evaluación ✓

---

## 📞 Soporte Rápido

```bash
# Ver opciones de make
make -C tests help

# Tests automatizados solo
./tests/quick_tests.sh

# Evaluación 42
python3 tests/eval_42.py

# Suite completa
python3 tests/test_suite.py

# Crear configs de test
./tests/setup_test_configs.sh
```

---

## 📝 Archivos Generados

- `test_report.json` - Reporte detallado en JSON
- `configs/test_configs/` - Configuraciones de test

---

**Created:** Abril 2026  
**For:** 42Webserv Project  
**Status:** ✅ Production Ready

