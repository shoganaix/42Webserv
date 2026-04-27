#!/bin/bash
# Script para crear configuraciones de test

set -e

# Colors
BLUE='\033[0;34m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║         WEBSERV TEST CONFIGURATION GENERATOR            ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}\n"

# Create test configs directory if it doesn't exist
mkdir -p configs/test_configs

# Test Config 1: Multiple Ports
echo -e "${YELLOW}Creating config: multiple_ports.conf${NC}"
cat > configs/test_configs/multiple_ports.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    host 0.0.0.0;
    root docs/fusion_web/;
    index index.html;
    
    location / {
        allow_methods GET;
    }
}

server {
    listen 8081;
    server_name localhost;
    host 0.0.0.0;
    root docs/;
    index index.html;
    
    location / {
        allow_methods GET;
    }
}
EOF
echo -e "${GREEN}✓ Created${NC}"

# Test Config 2: Methods Restriction
echo -e "${YELLOW}Creating config: methods_restriction.conf${NC}"
cat > configs/test_configs/methods_restriction.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    host 0.0.0.0;
    root docs/fusion_web/;
    index index.html;
    
    location / {
        allow_methods GET;
    }
    
    location /post_only {
        allow_methods POST;
        client_max_body_size 1000;
    }
    
    location /delete_allowed {
        allow_methods GET DELETE;
    }
}
EOF
echo -e "${GREEN}✓ Created${NC}"

# Test Config 3: Custom Error Pages
echo -e "${YELLOW}Creating config: error_pages.conf${NC}"
cat > configs/test_configs/error_pages.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    host 0.0.0.0;
    root docs/fusion_web/;
    index index.html;
    
    error_page 404 /error_pages/404.html;
    error_page 405 /error_pages/405.html;
    error_page 500 /error_pages/500.html;
    
    location / {
        allow_methods GET POST;
    }
}
EOF
echo -e "${GREEN}✓ Created${NC}"

# Test Config 4: CGI Testing
echo -e "${YELLOW}Creating config: cgi_test.conf${NC}"
cat > configs/test_configs/cgi_test.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    host 0.0.0.0;
    root docs/fusion_web/;
    index index.html;
    
    location / {
        allow_methods GET;
    }
    
    location /cgi-bin/ {
        allow_methods GET POST;
        cgi_path ../cgi_tester;
        cgi_ext .bla .py;
    }
}
EOF
echo -e "${GREEN}✓ Created${NC}"

# Test Config 5: Alias and Index
echo -e "${YELLOW}Creating config: alias_test.conf${NC}"
cat > configs/test_configs/alias_test.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    host 0.0.0.0;
    root docs/fusion_web/;
    index index.html;
    
    location / {
        allow_methods GET;
    }
    
    location /files/ {
        alias ./YoupiBanane/;
        index youpi.bad_extension;
        allow_methods GET;
        autoindex off;
    }
}
EOF
echo -e "${GREEN}✓ Created${NC}"

# Test Config 6: Strict Body Limit
echo -e "${YELLOW}Creating config: body_limit.conf${NC}"
cat > configs/test_configs/body_limit.conf << 'EOF'
server {
    listen 8080;
    server_name localhost;
    host 0.0.0.0;
    root docs/fusion_web/;
    index index.html;
    
    client_max_body_size 1000;
    
    location / {
        allow_methods GET;
    }
    
    location /small_upload {
        allow_methods POST;
        client_max_body_size 100;
        upload_path ./uploads;
    }
    
    location /medium_upload {
        allow_methods POST;
        client_max_body_size 10000;
        upload_path ./uploads;
    }
}
EOF
echo -e "${GREEN}✓ Created${NC}"

# Test Config 7: Multiple Hostnames (local test)
echo -e "${YELLOW}Creating config: hostnames.conf${NC}"
cat > configs/test_configs/hostnames.conf << 'EOF'
server {
    listen 8080;
    server_name example.com;
    host 0.0.0.0;
    root docs/fusion_web/;
    index index.html;
}

server {
    listen 8080;
    server_name another.example.com;
    host 0.0.0.0;
    root docs/;
    index index.html;
}
EOF
echo -e "${GREEN}✓ Created${NC}"

echo ""
echo -e "${BLUE}╔════════════════════════════════════════════════════════╗${NC}"
echo -e "${BLUE}║               Configuration Files Created               ║${NC}"
echo -e "${BLUE}╚════════════════════════════════════════════════════════╝${NC}"
echo ""
echo "📁 Configs created in: configs/test_configs/"
echo ""
echo "Usage:"
echo "  ./webserv configs/test_configs/multiple_ports.conf"
echo "  ./webserv configs/test_configs/methods_restriction.conf"
echo "  ./webserv configs/test_configs/error_pages.conf"
echo "  ./webserv configs/test_configs/cgi_test.conf"
echo "  ./webserv configs/test_configs/alias_test.conf"
echo "  ./webserv configs/test_configs/body_limit.conf"
echo "  ./webserv configs/test_configs/hostnames.conf"
echo ""
