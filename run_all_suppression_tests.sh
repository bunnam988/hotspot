#!/bin/sh
# run_all_suppression_tests.sh — Run all 10 suppression test modes on device
# Usage: sh run_all_suppression_tests.sh

LOGFILE="/rdklogs/logs/Hotspotlog.txt.0"

run_test() {
    trigger="$1"
    wait_sec="$2"
    desc="$3"

    echo "----------------------------------------"
    echo "[TEST] $desc"
    echo "  touch $trigger -> wait ${wait_sec}s -> rm"
    touch "$trigger"
    sleep "$wait_sec"
    rm -f "$trigger"
    echo "  Done. Waiting 35s for next cycle..."
    sleep 35
}

echo "============================================================"
echo "  Log Suppression Test Runner"
echo "============================================================"
echo ""
echo "Log file: $LOGFILE"
echo "Start: $(date)"
echo ""

# Mark start position in log
echo "--- TEST START $(date) ---" >> "$LOGFILE" 2>/dev/null || true
START_LINE=$(wc -l < "$LOGFILE" 2>/dev/null || echo 0)

# Mode 1: Single burst (~100 msg/s for 5s)
run_test /tmp/log_flood 10 "Mode 1: Single+burst (~100 msg/s, 5s)"

# Mode 3: Multi burst (3-msg pattern at ~100 msg/s for 5s)
run_test /tmp/log_pattern_3 10 "Mode 3: Multi+burst (3-msg pattern, 5s)"

# Mode 11: Periodic single (same msg every 2s for 30s)
run_test /tmp/log_periodic 35 "Mode 11: Single+periodic (every 2s, 30s)"

# Mode 12: Periodic pattern (3-msg every 3s for 30s)
run_test /tmp/log_periodic_pattern 35 "Mode 12: Multi+periodic ms (3-msg, every 3s)"

# Mode 13: Sporadic single (random 1-8s for 60s)
run_test /tmp/log_sporadic 65 "Mode 13: Single+sporadic (random 1-8s, 60s)"

# Mode 14: Short burst (50 msgs in 200ms)
run_test /tmp/log_short_burst 5 "Mode 14: Single+burst at-window (50 msgs, 200ms)"

# Mode 16: Sporadic pattern (2-msg, random 3-10s, 60s)
run_test /tmp/log_sporadic_pattern 65 "Mode 16: Multi+sporadic (2-msg, random gaps, 60s)"

# Mode 17: Periodic pattern sec (3-msg, 6s cycle, 42s)
run_test /tmp/log_periodic_pattern_sec 50 "Mode 17: Multi+periodic sec (3-msg, 6s cycle)"

# Mode 18: Periodic varied (4-msg, non-uniform, 5s cycle, 35s)
run_test /tmp/log_periodic_pattern_varied 40 "Mode 18: Multi+periodic varied (4-msg, 5s cycle)"

# Mode 19: Burst varied (3-msg, non-uniform, 3s)
run_test /tmp/log_burst_pattern_varied 8 "Mode 19: Multi+burst varied (3-msg, 3s)"

echo ""
echo "============================================================"
echo "  ALL TESTS COMPLETE"
echo "============================================================"
echo "End: $(date)"
echo ""

# Show summary from log
echo "Results from log:"
SUPPRESS_COUNT=$(tail -n +"$START_LINE" "$LOGFILE" 2>/dev/null | grep -c "SUPPRESS" || echo 0)
AT_COUNT=$(tail -n +"$START_LINE" "$LOGFILE" 2>/dev/null | grep -c "At:" || echo 0)
TOTAL_LINES=$(tail -n +"$START_LINE" "$LOGFILE" 2>/dev/null | wc -l || echo 0)
echo "  Total lines:    $TOTAL_LINES"
echo "  [SUPPRESS] lines: $SUPPRESS_COUNT"
echo "  At: lines:      $AT_COUNT"
echo ""

# Save test output for offline analysis
tail -n +"$START_LINE" "$LOGFILE" > /tmp/suppression_test_output.log 2>/dev/null
echo "Full output saved: /tmp/suppression_test_output.log"
echo "Copy it off-device to run expand_suppressed_logs.py"
