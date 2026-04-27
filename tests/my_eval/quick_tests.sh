#!/bin/bash
# Quick automated tests for webserv

set -e

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Variables
WEBSERV_BINARY="./webserv"
CONFIG_FILE="./configs/default.conf"
HOST="127.0.0.1"
PORT=8080
PID=""

# Functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[✓]${NC} $1"
}

log_error() {
    echo -e "${RED}[✗]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[!]${NC} $1"
}

start_server() {
    log_info "Starting server..."
    $WEBSERV_BINARY $CONFIG_FILE &
    PID=$!
    sleep 2
    
    if ps -p $PID > /dev/null; then
        log_success "Server started (PID: $PID)"
        return 0
    else
        log_error "Failed to start server"
        return 1
    fi
}

stop_server() {
    if [ -n "$PID" ]; then
        log_info "Stopping server (PID: $PID)..."
        kill $PID 2>/dev/null || true
        sleep 1
        kill -9 $PID 2>/dev/null || true
        log_success "Server stopped"
    fi
}

# Trap to ensure server stops
trap stop_server EXIT

test_get_request() {
    log_info "Testing GET request..."
    
    response=$(curl -s -w "\n%{http_code}" http://$HOST:$PORT/)
    status=$(echo "$response" | tail -n1)
    
    if [ "$status" = "200" ]; then
        log_success "GET request returned 200"
        return 0
    else
        log_error "GET request returned $status (expected 200)"
        return 1
    fi
}

test_post_request() {
    log_info "Testing POST request..."
    
    response=$(curl -s -w "\n%{http_code}" -X POST \
        -H "Content-Type: text/plain" \
        --data "test data" \
        http://$HOST:$PORT/post_body)
    status=$(echo "$response" | tail -n1)
    
    if [ "$status" = "200" ]; then
        log_success "POST request returned 200"
        return 0
    else
        log_error "POST request returned $status (expected 200)"
        return 1
    fi
}

test_delete_forbidden() {
    log_info "Testing DELETE (should be forbidden)..."
    
    response=$(curl -s -w "\n%{http_code}" -X DELETE http://$HOST:$PORT/)
    status=$(echo "$response" | tail -n1)
    
    if [ "$status" = "405" ] || [ "$status" = "403" ]; then
        log_success "DELETE correctly forbidden ($status)"
        return 0
    else
        log_warning "DELETE returned $status (expected 405 or 403)"
        return 1
    fi
}

test_unknown_method() {
    log_info "Testing unknown HTTP method (should not crash)..."
    
    response=$(timeout 5 curl -s -X BADMETHOD http://$HOST:$PORT/ 2>&1 || true)
    
    if [ -n "$response" ]; then
        log_success "Server handled unknown method without crashing"
        return 0
    else
        log_error "Server might have crashed"
        return 1
    fi
}

test_404_error() {
    log_info "Testing 404 error..."
    
    response=$(curl -s -w "\n%{http_code}" http://$HOST:$PORT/nonexistent12345)
    status=$(echo "$response" | tail -n1)
    
    if [ "$status" = "404" ]; then
        log_success "404 error returned correctly"
        return 0
    else
        log_error "Expected 404, got $status"
        return 1
    fi
}

test_body_size_limit() {
    log_info "Testing body size limit..."
    
    # Small body (should work)
    small_data=$(printf 'x%.0s' {1..50})
    response=$(curl -s -w "\n%{http_code}" -X POST \
        --data "$small_data" \
        http://$HOST:$PORT/post_body)
    status=$(echo "$response" | tail -n1)
    
    if [ "$status" = "200" ]; then
        log_success "Small body (50 bytes) accepted"
    else
        log_error "Small body returned $status"
        return 1
    fi
    
    # Large body (should fail with 413)
    large_data=$(printf 'x%.0s' {1..500})
    response=$(curl -s -w "\n%{http_code}" -X POST \
        --data "$large_data" \
        http://$HOST:$PORT/post_body)
    status=$(echo "$response" | tail -n1)
    
    if [ "$status" = "413" ] || [ "$status" = "400" ]; then
        log_success "Large body (500 bytes) correctly rejected ($status)"
        return 0
    else
        log_error "Large body returned $status (expected 413 or 400)"
        return 1
    fi
}

test_compilation() {
    log_info "Testing compilation..."
    
    if make clean && make; then
        log_success "Compilation successful"
        return 0
    else
        log_error "Compilation failed"
        return 1
    fi
}

test_headers() {
    log_info "Testing response headers..."
    
    response=$(curl -s -i http://$HOST:$PORT/)
    
    if echo "$response" | grep -q "HTTP/1.1"; then
        log_success "HTTP version header present"
    else
        log_error "HTTP version header missing"
        return 1
    fi
    
    if echo "$response" | grep -q -i "Content-Type"; then
        log_success "Content-Type header present"
    else
        log_warning "Content-Type header missing"
    fi
    
    if echo "$response" | grep -q -i "Server"; then
        log_success "Server header present"
    else
        log_warning "Server header missing"
    fi
    
    return 0
}

# Main
main() {
    echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║          WEBSERV QUICK TEST SUITE                      ║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}\n"
    
    # Test compilation
    if ! test_compilation; then
        log_error "Cannot proceed without successful compilation"
        exit 1
    fi
    echo ""
    
    # Start server
    if ! start_server; then
        exit 1
    fi
    echo ""
    
    # Run tests
    tests=(
        "test_get_request"
        "test_post_request"
        "test_delete_forbidden"
        "test_unknown_method"
        "test_404_error"
        "test_body_size_limit"
        "test_headers"
    )
    
    passed=0
    failed=0
    
    for test in "${tests[@]}"; do
        if $test; then
            ((passed++))
        else
            ((failed++))
        fi
        echo ""
    done
    
    # Summary
    echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║                    TEST SUMMARY                        ║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"
    echo ""
    echo -e "Passed: ${GREEN}${passed}${NC}"
    echo -e "Failed: ${RED}${failed}${NC}"
    echo -e "Total:  $((passed + failed))"
    echo ""
    
    if [ $failed -eq 0 ]; then
        echo -e "${GREEN}All tests passed!${NC}"
        exit 0
    else
        echo -e "${RED}Some tests failed!${NC}"
        exit 1
    fi
}

# Run main
main
