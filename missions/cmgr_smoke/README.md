# cmgr_smoke

Minimal CI/smoke mission for `pContactMgrV20`.

- One simulated ownship transits left-to-right.
- A local `uTimerScript` posts a `SPOOF` event to `pSpoofNode`.
- `pAutoPoke` auto-deploys the vehicle on ordinary `launch.sh` and `zlaunch.sh` runs.
- `pContactMgrV20` should detect the spoofed contact and latch `CONTACT_SEEN=true`.
- `pMissionEval` always runs and passes when the ownship arrives and `CONTACT_SEEN=true`.

Typical runs:

```bash
./launch.sh 10
./launch.sh --nogui 10
./zlaunch.sh --nogui --max_time=120
```
