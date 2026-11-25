#!/bin/bash

# Script to run both SMP allocation strategies and collect results

echo "========================================================================="
echo "Multi-Core SMP Test Suite"
echo "========================================================================="
echo ""

# Test 1: Quadrant-Based Dedication
echo "[TEST 1] Running Quadrant-Based Core Dedication Strategy..."
echo "----------------------------------------------------------------------"
timeout 15 ./build/freertos_demo_smp 2>&1 > /tmp/dedication_results.txt
echo "✓ Dedication test complete"
echo ""

# Extract statistics for Dedication
echo "Extracting Dedication statistics..."
grep -A 30 "MULTI-CORE SYSTEM STATISTICS" /tmp/dedication_results.txt | tail -35 > /tmp/dedication_stats.txt

# Test 2: PU-WFD (need to rebuild)
echo "[TEST 2] Rebuilding with PU-WFD allocation strategy..."
echo "----------------------------------------------------------------------"

# Modify source to use PU-WFD
sed -i 's/#define USE_PU_WFD_ALLOCATION  0/#define USE_PU_WFD_ALLOCATION  1/' src/freertos_quadrant_demo_smp.c

# Rebuild
make clean > /dev/null 2>&1
make smp > /dev/null 2>&1

echo "Running PU-WFD Strategy..."
timeout 15 ./build/freertos_demo_smp 2>&1 > /tmp/pu_wfd_results.txt
echo "✓ PU-WFD test complete"
echo ""

# Extract statistics for PU-WFD
echo "Extracting PU-WFD statistics..."
grep -A 30 "MULTI-CORE SYSTEM STATISTICS" /tmp/pu_wfd_results.txt | tail -35 > /tmp/pu_wfd_stats.txt

# Restore original setting
sed -i 's/#define USE_PU_WFD_ALLOCATION  1/#define USE_PU_WFD_ALLOCATION  0/' src/freertos_quadrant_demo_smp.c

echo "========================================================================="
echo "Tests Complete! Results saved to:"
echo "  - /tmp/dedication_results.txt (full log)"
echo "  - /tmp/pu_wfd_results.txt (full log)"
echo "  - /tmp/dedication_stats.txt (statistics)"
echo "  - /tmp/pu_wfd_stats.txt (statistics)"
echo "========================================================================="
