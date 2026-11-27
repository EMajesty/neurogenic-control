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
