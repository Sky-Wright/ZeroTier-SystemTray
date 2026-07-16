#!/bin/bash
# Copyright (c) 2026 Skylar Hobdy-Wright. All Rights Reserved.
#
# MIT License backup script for the ZeroTier SystemTray project.

set -e

# Configuration
SOURCE_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
REMOTE_URL="git@github.com:Sky-Wright/ZeroTier-SystemTray.git"

echo "============================================"
echo "ZeroTier SystemTray Backup Script (Interactive)"
echo "============================================"

# Navigate to source
cd "$SOURCE_DIR" || { echo "Error: Could not change to $SOURCE_DIR"; exit 1; }

# Initialize git if not present
if [ ! -d ".git" ]; then
    echo "Initializing new git repository..."
    git init
    git branch -M main
fi

# Force Remote to SSH
if git remote | grep -q "origin"; then
    CURRENT_URL=$(git remote get-url origin)
    if [ "$CURRENT_URL" != "$REMOTE_URL" ]; then
        echo "Updating remote 'origin' to SSH..."
        git remote set-url origin "$REMOTE_URL"
    fi
else
    echo "Adding remote 'origin'..."
    git remote add origin "$REMOTE_URL"
fi

# Create or Update .gitignore if not present
if [ ! -f ".gitignore" ]; then
    echo "Creating .gitignore..."
    touch .gitignore
fi

# Function to ensure a pattern is in .gitignore
add_ignore() {
    local pattern="$1"
    if ! grep -qF "$pattern" .gitignore; then
        echo "$pattern" >> .gitignore
        echo "Added $pattern to .gitignore"
    fi
}

# Add standard build ignores
add_ignore "build/"
add_ignore "pkg/"
add_ignore "src/"
add_ignore "*.pkg.tar.zst"
add_ignore "*.log"
add_ignore ".DS_Store"

# Show status
echo ""
echo "============================================"
echo "Git Status:"
echo "============================================"
git status --short

# Check if there are changes (including untracked files)
if [ -z "$(git status --porcelain)" ]; then
    echo ""
    echo "No changes to commit."
    exit 0
fi

# Prompt for commit message
echo ""
echo "============================================"
echo "Enter commit message:"
echo "(Press Enter for default: 'Backup: ZeroTier SystemTray - [date]')"
echo "============================================"
read -p "> " COMMIT_MSG

# Use default if empty
if [ -z "$COMMIT_MSG" ]; then
    COMMIT_MSG="Backup: ZeroTier SystemTray - $(date '+%Y-%m-%d %H:%M:%S')"
    echo "Using default message: $COMMIT_MSG"
fi

# Stage and Commit
echo ""
echo "Staging files..."
git add .

echo "Committing changes..."
git commit -m "$COMMIT_MSG"
echo "Changes committed."

# Confirm push
echo ""
echo "============================================"
read -p "Push to GitHub? (Y/n): " CONFIRM_PUSH
CONFIRM_PUSH=${CONFIRM_PUSH:-Y}

if [[ "$CONFIRM_PUSH" =~ ^[Yy]$ ]]; then
    echo "Pushing to GitHub (SSH)..."
    git push -u origin main --force
    echo ""
    echo "============================================"
    echo "Backup Complete!"
    echo "============================================"
else
    echo "Push cancelled. Changes committed locally only."
fi
