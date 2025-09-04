#!/bin/bash

# Script to fix VERIFY_CONSERVATION_WITH_WITNESS macro calls
# Convert them to direct code to avoid macro expansion issues

files=(
    "core/shard_benchmarks.c"
    "geometric/manifold_benchmarks.c" 
    "geometric/transform_benchmarks.c"
    "traditional/algorithm_benchmarks.c"
    "applications/computational_benchmarks.c"
)

for file in "${files[@]}"; do
    echo "Processing $file..."
    
    # Create a backup
    cp "$file" "$file.bak"
    
    # Use sed to replace macro calls with direct code
    # This is a more conservative approach - we'll manually fix each file
    echo "Backup created: $file.bak"
done

echo "All files backed up. Manual fixes needed for each file."