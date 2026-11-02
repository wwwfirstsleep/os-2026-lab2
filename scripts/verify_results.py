#!/usr/bin/env python3
"""Validate actual CSV output; standard library only."""
import csv, sys
from pathlib import Path

def expected(loops):
    total = 0
    for seed in (24281204, 24281204 + 12345):
        x = seed
        for _ in range(loops):
            x ^= (x << 13) & 0xffffffff
            x ^= x >> 17
            x ^= (x << 5) & 0xffffffff
            total += x % 100 + 1
    return total

cache = {}
for filename in sys.argv[1:]:
    rows = list(csv.DictReader(line for line in Path(filename).read_text().splitlines() if not line.startswith('#')))
    assert rows, 'empty output'
    failures = 0
    for r in rows:
        n = int(r['loops'])
        if r['mode'] == 'none':
            failures += int(r['detected'])
            continue
        if n not in cache:
            cache[n] = expected(n)
        assert int(r['count0']) == n and int(r['count1']) == n, r
        assert int(r['account1']) == cache[n], r
        assert int(r['account2']) == -cache[n], r
        assert int(r['final_sum']) == 0 and int(r['detected']) == 0, r
        assert int(r['complete']) == 1 and float(r['wall_ms']) > 0, r
    print(f'PASS {filename}: {len(rows)} rows; synchronized totals verified; none detections={failures}')
