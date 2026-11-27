# Standalone USB diagnostics

USB CDC text diagnostics are separate from the future binary protocol. Commands
are limited to 63 characters: `help`, `version`, `hardware`, `ports`, `snapshot`,
`raw`, `buttons`, `encoders`, `stats`, `stream on`, `stream off`, `reset-stats`.
Invalid/long commands return errors. Streaming is capped at 10 Hz and scanning
never waits for input. HW-002 has no illumination commands.

```sh
python3 tools/neurogenic_diag.py --device /dev/ttyACM0 --command snapshot
python3 tools/neurogenic_diag.py --device /dev/ttyACM0 --command 'stream on' \
  --seconds 30 --max-bytes 1000000 --record bringup.log
```

The device path is mandatory; arbitrary serial devices are never opened. A
plain terminal remains sufficient. Slow consumers may lose diagnostic output,
but do not block scanning; inspect stats afterward.

## Safe LED diagnostics

Confirm the external current-limited 5 V LED rail before `led enable`; USB power alone is insufficient and no rail-sense input exists. Commands include `led status`, `led pixel I R G B`, `led encoder I R G B`, `led channel I VALUE`, `led brightness V`, the three `led walk` variants, `led test rgb`, `led test white`, `led stop`, `led off`, and `led disable`. Pixel, encoder and TLC indices are 0..57, 0..7 and 0..23. Animations are nonblocking and every request uses the same limiter.

```sh
python3 tools/neurogenic_diag.py --device /dev/ttyACM0 --command 'led status'
python3 tools/neurogenic_diag.py --device /dev/ttyACM0 --command 'led pixel 0 255 0 0'
```

Finish with `led off`, allow the frame/reset interval, then `led disable`. Do not hot-plug modules.
