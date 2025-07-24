#!/bin/bash
#set environment variable set
#source ./scripts/config.sh

# Step 1: Clean the build
echo "Cleaning the project..."
rm -rf bin buffer_data
mkdir -p bin buffer_data && touch buffer_data/focas_buffer.json
make clean

# Step 2: Build the project
echo "Building the project..."
make

# Step 6: Verify dependencies
echo "Verifying dependencies..."
ldd bin/iot_connector

make run

