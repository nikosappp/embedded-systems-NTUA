#!/bin/bash
set -euo pipefail

INPUTS=("rand_str_input_first.txt" "rand_str_input_sec.txt")
SUFFIXES=("_len_out" "_concat_out" "_sorted_out")
FUNCS=("strlen" "strcpy" "strcat" "strcmp")

# Create a private temp directory in /tmp and guarantee cleanup on exit
TMP_DIR=$(mktemp -d /tmp/q3_ref.XXXXXX)
trap 'rm -rf "$TMP_DIR"' EXIT

echo "=== 1. Clean & Rebuild ==="
make clean
rm -f *_len_out *_concat_out *_sorted_out
make

echo ""
echo "=== 2. Verify Custom ARM Assembly Symbols ==="
for fn in "${FUNCS[@]}"; do
    if nm string_manipulation.out | grep -qE " T ${fn}$"; then
        echo "[PASS] ${fn} is locally defined (T) in string_manipulation.out"
    else
        echo "[FAIL] ${fn} is NOT locally defined in string_manipulation.out"
        exit 1
    fi
done

echo ""
echo "=== 3. Run string_manipulation.out on Both Inputs ==="
for infile in "${INPUTS[@]}"; do
    ./string_manipulation.out "$infile"
done
ls -lh *_len_out *_concat_out *_sorted_out

echo ""
echo "=== 4. Build & Run glibc Reference in ${TMP_DIR} ==="
gcc -Wall string_manipulation.c -o "${TMP_DIR}/ref.out"
for infile in "${INPUTS[@]}"; do
    cp "$infile" "${TMP_DIR}/${infile}"
    "${TMP_DIR}/ref.out" "${TMP_DIR}/${infile}"
done
ls -lh "${TMP_DIR}"/*_out

echo ""
echo "=== 5. Compare Assembly Outputs vs glibc Reference ==="
FAILED=0
for infile in "${INPUTS[@]}"; do
    for suffix in "${SUFFIXES[@]}"; do
        asm_out="${infile}${suffix}"
        ref_out="${TMP_DIR}/${infile}${suffix}"
        if diff -s "$asm_out" "$ref_out"; then
            :
        else
            FAILED=1
        fi
    done
done

echo ""
if [ "$FAILED" -eq 0 ]; then
    echo "ALL TESTS PASSED!"
else
    echo "SOME TESTS FAILED!"
    exit 1
fi
