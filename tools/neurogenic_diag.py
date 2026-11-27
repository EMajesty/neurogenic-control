#!/usr/bin/env python3
"""Bounded dependency-free Linux client for the diagnostic CDC interface."""
import argparse, os, re, select, sys, termios, time
COMMANDS={'version','hardware','ports','snapshot','raw','buttons','encoders','stats','stream on','stream off','reset-stats','help','led enable','led disable','led off','led status','led walk pixels','led walk encoders','led walk channels','led test rgb','led test white','led stop','led render status','led render stop','led render gamma linear','led render gamma quadratic','led calibration reset','led calibration export','led calibration ramp','led calibration compare'}
LED_VALUE=re.compile(r'led (pixel|encoder) ([0-9]+) ([0-9]+) ([0-9]+) ([0-9]+)$|led channel ([0-9]+) ([0-9]+)$|led brightness ([0-9]+)$')
def valid_command(command):
    if command in COMMANDS:return True
    p=command.split()
    try:
        if p[:3]==['led','render','encoder'] and len(p)==8:return 0<=int(p[3])<8 and all(0<=int(x)<256 for x in p[4:7]) and 0<=int(p[7])<65536
        if p[:3]==['led','render','bipolar'] and len(p)==8:return 0<=int(p[3])<8 and all(0<=int(x)<256 for x in p[4:7]) and -32768<=int(p[7])<=32767
        if p[:3] in (['led','render','unassigned'],['led','render','unavailable']) and len(p)==4:return 0<=int(p[3])<8
        if p[:3]==['led','render','minimum'] and len(p)==4:return 0<=int(p[3])<=128
        if p[:3]==['led','render','transition'] and len(p)==4:return 0<=int(p[3])<=1000
        if p[:3]==['led','render','animation'] and len(p)==5:return 0<=int(p[3])<8 and 0<=int(p[4])<=5
        if p[:3]==['led','calibration','driver'] and len(p)==7:return 0<=int(p[3])<2 and all(0<=int(x)<=512 for x in p[4:])
        if p[:3]==['led','calibration','encoder'] and len(p)==7:return 0<=int(p[3])<8 and all(0<=int(x)<=512 for x in p[4:])
        if p[:3]==['led','calibration','pixel'] and len(p)==7:return 0<=int(p[3])<58 and all(0<=int(x)<=512 for x in p[4:])
    except ValueError:return False
    m=LED_VALUE.fullmatch(command)
    if not m:return False
    values=[int(x) for x in m.groups()[1:] if x is not None]
    if command.startswith('led pixel '):return values[0]<58 and all(x<256 for x in values[1:])
    if command.startswith('led encoder '):return values[0]<8 and all(x<256 for x in values[1:])
    if command.startswith('led channel '):return values[0]<24 and values[1]<4096
    return values[0]<256
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
    ap=argparse.ArgumentParser();ap.add_argument('--device',required=True,help='explicit /dev/ttyACM* path');ap.add_argument('--command',default='version',help='bounded diagnostic command, including render/calibration commands');ap.add_argument('--seconds',type=float,default=.5);ap.add_argument('--max-bytes',type=int,default=65536);ap.add_argument('--record');ap.add_argument('--all-off-on-exit',action='store_true',help='best-effort led off before closing; not a cable-disconnect guarantee');a=ap.parse_args(argv)
    if not valid_command(a.command) or len(a.command)>63:ap.error('invalid or out-of-range diagnostic command')
    if not a.device.startswith('/dev/') or not os.path.exists(a.device):ap.error('device must be an existing explicit /dev path')
    if not 0<a.seconds<=3600 or not 1<=a.max_bytes<=10_000_000:ap.error('bounded seconds/max-bytes required')
    out=open(a.record,'wb') if a.record else None;fd=open_device(a.device)
    try:
        if out:
            class W:pass
            w=W();w.buffer=out;w.flush=out.flush;return transact(fd,a.command,a.seconds,a.max_bytes,w)
        return 0 if transact(fd,a.command,a.seconds,a.max_bytes,sys.stdout)>=0 else 1
    finally:
        if a.all_off_on_exit:
            try:os.write(fd,b'led off\n')
            except OSError:pass
        os.close(fd);out and out.close()
if __name__=='__main__':raise SystemExit(main())
