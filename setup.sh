#!/bin/bash

# Set Git hooks path to use the shared hooks in .githooks
git config core.hooksPath .githooks

# Make sure all hooks are executable
chmod +x .githooks/*

# Output a success message
echo "Git hooks path set to .githooks, and all hooks are now executable."
