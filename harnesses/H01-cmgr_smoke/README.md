# H01-cmgr_smoke

Patch-driven harness for `missions/cmgr_smoke`.

- `baseline_pass`: stock contact-manager smoke case, expected pass
- `strict_fail`: tighter contact range, expected fail
- `strict_near_pass`: tighter contact range plus nearer spoof contact, expected pass

Typical runs:

```bash
./zlaunch.sh
./zlaunch.sh --case=strict_fail
./zlaunch.sh --max_time=80 10
```
