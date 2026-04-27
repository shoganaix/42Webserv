# Makefile para tests de webserv

.PHONY: test quick-test eval-test stress-test manual-test clean help

help:
	@echo "╔═══════════════════════════════════════════════╗"
	@echo "║    WEBSERV TEST SUITE - Available Commands    ║"
	@echo "╚═══════════════════════════════════════════════╝"
	@echo ""
	@echo "make quick-test      - Pruebas rápidas (5 min)"
	@echo "make eval-test       - Pruebas de evaluación 42 (30 min)"
	@echo "make test            - Suite completa (45 min)"
	@echo "make stress-test     - Test de estrés con siege"
	@echo "make manual-test     - Guía de pruebas manuales"
	@echo "make clean           - Limpiar logs de tests"
	@echo "make help            - Mostrar esta ayuda"
	@echo ""

# Pruebas rápidas automáticas
quick-test:
	@echo "🚀 Ejecutando pruebas rápidas..."
	@bash tests/quick_tests.sh
	@echo "✓ Pruebas completadas"

# Pruebas de evaluación 42
eval-test:
	@echo "🎯 Ejecutando pruebas de evaluación 42..."
	@python3 tests/eval_42.py
	@echo "✓ Evaluación completada"

# Suite completa
test:
	@echo "📋 Ejecutando suite completa..."
	@python3 tests/test_suite.py
	@echo "✓ Suite completada"

# Stress test con siege
stress-test:
	@echo "🔥 Iniciando stress test con siege..."
	@which siege > /dev/null || (echo "⚠️  Siege no instalado. Instalando..."; sudo apt-get install siege -y)
	@./webserv ./configs/default.conf &
	@sleep 2
	@siege -b -u http://127.0.0.1:8080/ -t 60S
	@pkill -f webserv || true

# Test manual guideado
manual-test:
	@echo "📖 Abriendo guía de pruebas manuales..."
	@less tests/TEST_GUIDE.md

# Logs
clean:
	@rm -f test_report.json
	@rm -f logs/*.log
	@echo "✓ Logs limpiados"

.DEFAULT_GOAL := help
