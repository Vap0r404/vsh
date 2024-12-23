#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}Installing VSH...${NC}"

# Compile the shell
gcc -o vsh v_shell.c

if [ $? -ne 0 ]; then
    echo -e "${RED}Compilation failed!${NC}"
    exit 1
fi

# Copy to system binary directory
sudo cp vsh /usr/local/bin/

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to copy VSH to /usr/local/bin/${NC}"
    exit 1
fi

# Set proper permissions
sudo chmod 755 /usr/local/bin/vsh

# Add to /etc/shells if not already present
if ! grep -q "/usr/local/bin/vsh" /etc/shells; then
    echo "/usr/local/bin/vsh" | sudo tee -a /etc/shells
fi

echo -e "${GREEN}VSH installed successfully!${NC}"
echo "You can now:"
echo "1. Run 'vsh' to start the shell"
echo "2. Set it as your default shell with: chsh -s /usr/local/bin/vsh"
