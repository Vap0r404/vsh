#!/bin/bash

# Colors for output
GREEN='\033[0;32m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${GREEN}Uninstalling VSH...${NC}"

# Remove VSH from /usr/local/bin
sudo rm -f /usr/local/bin/vsh

if [ $? -ne 0 ]; then
    echo -e "${RED}Failed to remove VSH from /usr/local/bin/${NC}"
    exit 1
fi

# Remove from /etc/shells
sudo sed -i '/\/usr\/local\/bin\/vsh/d' /etc/shells

echo -e "${GREEN}VSH uninstalled successfully!${NC}"
