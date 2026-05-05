#!/bin/bash
# run_all.sh - Runs all Producer-Consumer configurations from input_config.txt

echo "======================================================"
echo "  Producer-Consumer - Running All Configurations"
echo "======================================================"

while IFS= read -r line || [[ -n "$line" ]]; do
    # Skip empty lines and comments
    [[ -z "$line" || "$line" == \#* ]] && continue
    read -r P C BUF SLEEP OUT <<< "$line"
    echo ""
    echo "--- P=$P  C=$C  buf=$BUF  sleep=${SLEEP}ms ---"
    ./producer_consumer "$P" "$C" "$BUF" "$SLEEP" "$OUT"
done < input_config.txt

echo ""
echo "======================================================"
echo "  All runs complete. Output files:"
ls output_*.txt
echo "======================================================"
