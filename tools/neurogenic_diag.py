#!/usr/bin/env python3
"""Bounded dependency-free Linux client for the diagnostic CDC interface."""
import argparse, os, select, sys, termios, time
COMMANDS={'version','hardware','ports','snapshot','raw','buttons','encoders','stats','stream on','stream off','reset-stats','help'}
def open_device(path):
    fd=os.open(path,os.O_RDWR|os.O_NOCTTY|os.O_NONBLOCK); attrs=termios.tcgetattr(fd);attrs[0]=attrs[1]=attrs[3]=0;attrs[2]=termios.CS8|termios.CREAD|termios.CLOCAL;termios.tcsetattr(fd,termios.TCSANOW,attrs);return fd
def transact(fd,command,duration,limit,out):
    os.write(fd,(command+'\n').encode());end=time.monotonic()+duration;used=0
    while time.monotonic()<end and used<limit:
        ready,_,_=select.select([fd],[],[],min(.1,end-time.monotonic()))
        if not ready:continue
        data=os.read(fd,min(4096,limit-used));
        if not data:continue
        used+=len(data);out.buffer.write(data);out.flush()
    return used
def main(argv=None):
    ap=argparse.ArgumentParser();ap.add_argument('--device',required=True,help='explicit /dev/ttyACM* path');ap.add_argument('--command',choices=sorted(COMMANDS),default='version');ap.add_argument('--seconds',type=float,default=.5);ap.add_argument('--max-bytes',type=int,default=65536);ap.add_argument('--record');a=ap.parse_args(argv)
    if not a.device.startswith('/dev/') or not os.path.exists(a.device):ap.error('device must be an existing explicit /dev path')
    if not 0<a.seconds<=3600 or not 1<=a.max_bytes<=10_000_000:ap.error('bounded seconds/max-bytes required')
    out=open(a.record,'wb') if a.record else None;fd=open_device(a.device)
    try:
        if out:
            class W:pass
            w=W();w.buffer=out;w.flush=out.flush;return transact(fd,a.command,a.seconds,a.max_bytes,w)
        return 0 if transact(fd,a.command,a.seconds,a.max_bytes,sys.stdout)>=0 else 1
    finally:os.close(fd);out and out.close()
if __name__=='__main__':raise SystemExit(main())
