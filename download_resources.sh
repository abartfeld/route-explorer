#!/bin/bash

# Script to download required resource files
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
RESOURCES_DIR="$SCRIPT_DIR/resources"

# Create fonts directory if it doesn't exist
mkdir -p "$RESOURCES_DIR/fonts"

# Download Roboto fonts if they don't exist
if [ ! -f "$RESOURCES_DIR/fonts/Roboto-Regular.ttf" ]; then
    echo "Downloading Roboto fonts..."
    
    # Download individual font files directly from Google Fonts CDN
    # This is more reliable than trying to download the entire family ZIP
    wget -q -O "$RESOURCES_DIR/fonts/Roboto-Regular.ttf" "https://fonts.gstatic.com/s/roboto/v30/KFOmCnqEu92Fr1Mu4mxK.ttf"
    wget -q -O "$RESOURCES_DIR/fonts/Roboto-Bold.ttf" "https://fonts.gstatic.com/s/roboto/v30/KFOlCnqEu92Fr1MmWUlfBBc9.ttf"
    wget -q -O "$RESOURCES_DIR/fonts/Roboto-Light.ttf" "https://fonts.gstatic.com/s/roboto/v30/KFOlCnqEu92Fr1MmSU5fBBc9.ttf"
    
    # Check if files were downloaded successfully
    if [ -f "$RESOURCES_DIR/fonts/Roboto-Regular.ttf" ] && 
       [ -f "$RESOURCES_DIR/fonts/Roboto-Bold.ttf" ] && 
       [ -f "$RESOURCES_DIR/fonts/Roboto-Light.ttf" ]; then
        echo "Roboto fonts downloaded successfully."
    else
        echo "ERROR: Failed to download Roboto fonts."
        echo "Please download the Roboto fonts manually from https://fonts.google.com/specimen/Roboto"
        echo "and place them in $RESOURCES_DIR/fonts/"
    fi
else
    echo "Roboto fonts already exist."
fi

# Make scripts executable
chmod +x "$SCRIPT_DIR/build/update_resources.sh"

echo "All resources have been configured."
