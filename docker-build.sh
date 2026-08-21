#!/bin/bash
set -e

# Build Docker image
echo "Building Docker image..."
docker build -t 3dslibris-dev .

# Run build inside container
echo "Running build inside Docker container..."
docker run --rm \
    -v "$(pwd):/project" \
    --platform linux/amd64 \
    3dslibris-dev \
    bash -c "cd /project && make clean && make"

echo "Build complete!"
