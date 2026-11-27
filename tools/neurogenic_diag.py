#!/usr/bin/env python3
"""Bounded dependency-free Linux client for the diagnostic CDC interface."""
import argparse, json, os, pathlib, re, select, sys, termios, time
import diagnostic_workflow as workflow
COMMANDS={'info','inventory','version','hardware','ports','snapshot','raw','buttons','encoders','stats','stream on','stream off','reset-stats','heartbeat','stress stop','stress status','help','led enable','led disable','led off','led status','led walk pixels','led walk encoders','led walk channels','led test rgb','led test white','led stop','led render status','led render stop','led render gamma linear','led render gamma quadratic','led calibration reset','led calibration export','led calibration ramp','led calibration compare'}
LED_VALUE=re.compile(r'led (pixel|encoder) ([0-9]+) ([0-9]+) ([0-9]+) ([0-9]+)$|led channel ([0-9]+) ([0-9]+)$|led brightness ([0-9]+)$')
def valid_command(command):
    if command in COMMANDS:return True
    p=command.split()
    try:
        if p[:1]==['module'] and len(p)==2:return 0<=int(p[1])<9
        if p[:2]==['stress','start'] and len(p)==3:return 1<=int(p[2])<=300
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
def collect(fd,command,duration,limit):
    os.write(fd,(command+'\n').encode());end=time.monotonic()+duration;data=bytearray()
    while time.monotonic()<end and len(data)<limit:
        ready,_,_=select.select([fd],[],[],min(.1,end-time.monotonic()))
        if not ready:continue
        chunk=os.read(fd,min(4096,limit-len(data)))
        if chunk:data.extend(chunk)
    return bytes(data)
def negotiate(fd,duration):
    raw=collect(fd,'info',duration,4096)
    try:lines=[x for x in raw.decode('ascii').splitlines() if x.startswith('NGD1 ')]
    except UnicodeDecodeError as exc:raise ValueError('non-ASCII diagnostic identity') from exc
    parsed=[workflow.parse_ngd1(x) for x in lines]
    identity=next((x for x in parsed if x.get('type')=='identity'),None)
    capabilities=next((x for x in parsed if x.get('type')=='capabilities'),None)
    if identity is None or capabilities is None:raise ValueError('incomplete diagnostic identity/capabilities')
    workflow.validate_identity(identity)
    return identity,set(capabilities.get('values','').split(','))
ACTION_COMMAND={'info':'info','monitor':'stream on','raw':'raw','buttons':'buttons',
                'encoders':'encoders','leds':'led status',
                'calibrate':'led calibration export','stress':'stress status'}

def write_json(path, value):
    pathlib.Path(path).write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')

def _offline_action(a):
    if a.action=='report':
        if not a.report:raise ValueError('--report is required')
        report=workflow.load_report(pathlib.Path(a.report))
        text=workflow.markdown(report)
        if a.output:pathlib.Path(a.output).write_text(text)
        else:sys.stdout.write(text)
        return 0
    if a.action=='harness':
        if not a.profile or not a.output:raise ValueError('--profile and --output are required')
        write_json(a.output,workflow.propose_harness(a.profile));return 0
    if a.action=='encoders' and a.encoder and a.output:
        if a.cw_raw is None or a.ccw_raw is None or a.known_steps is None:raise ValueError('--cw-raw, --ccw-raw and --known-steps are required')
        write_json(a.output,workflow.propose_encoder(a.encoder,a.cw_raw,a.ccw_raw,a.known_steps));return 0
    if a.action=='calibrate' and a.calibration_source and a.output:
        source=json.loads(pathlib.Path(a.calibration_source).read_text())
        write_json(a.output,workflow.propose_calibration(source));return 0
    if a.action=='bringup':
        path=pathlib.Path(a.report) if a.report else pathlib.Path('neurogenic-acceptance.json')
        report=workflow.load_report(path) if path.exists() else workflow.new_report()
        pending=next((x for x in report['stages'] if x['status']=='NOT RUN'),None)
        if not pending:return 0
        if not workflow.run_prerequisite(input,f"Complete stage {pending['id']} and confirm its stop conditions are clear"):
            workflow.set_stage(report,pending['id'],'BLOCKED',note='Operator did not confirm prerequisite.')
        else:
            result=input('Result [PASS/FAIL/BLOCKED/UNKNOWN]: ').strip().upper()
            workflow.set_stage(report,pending['id'],result if result in workflow.STATUSES else 'UNKNOWN')
        workflow.save_report(report,path);return 0
    return None
def main(argv=None):
    ap=argparse.ArgumentParser();ap.add_argument('action',nargs='?',default='command',choices=('command','info','monitor','raw','buttons','encoders','harness','leds','calibrate','stress','bringup','report'));ap.add_argument('--device',help='explicit /dev/ttyACM* path');ap.add_argument('--command',default='version',help='bounded diagnostic command, including render/calibration commands');ap.add_argument('--seconds',type=float,default=.5);ap.add_argument('--max-bytes',type=int,default=65536);ap.add_argument('--record');ap.add_argument('--report');ap.add_argument('--output');ap.add_argument('--profile',choices=tuple(workflow.module_profiles()));ap.add_argument('--encoder');ap.add_argument('--cw-raw',type=int);ap.add_argument('--ccw-raw',type=int);ap.add_argument('--known-steps',type=int);ap.add_argument('--calibration-source');ap.add_argument('--all-off-on-exit',action='store_true',help='best-effort led off before closing; not a cable-disconnect guarantee');a=ap.parse_args(argv)
    try:
        result=_offline_action(a)
    except (ValueError,OSError,json.JSONDecodeError) as exc:
        ap.error(str(exc))
    if result is not None:return result
    if not a.device:ap.error('--device is required for device actions')
    if a.action in ACTION_COMMAND:a.command=ACTION_COMMAND[a.action]
    if not valid_command(a.command) or len(a.command)>63:ap.error('invalid or out-of-range diagnostic command')
    if not a.device.startswith('/dev/') or not os.path.exists(a.device):ap.error('device must be an existing explicit /dev path')
    if not 0<a.seconds<=3600 or not 1<=a.max_bytes<=10_000_000:ap.error('bounded seconds/max-bytes required')
    out=open(a.record,'wb') if a.record else None;fd=open_device(a.device)
    try:
        if a.action not in ('command','info'):
            try:
                _,capabilities=negotiate(fd,min(a.seconds,1.0))
                needed={'monitor':'events','raw':'raw','buttons':'events','encoders':'encoders','leds':'illumination','calibrate':'calibration','stress':'stress'}[a.action]
                workflow.require_capability(capabilities,needed)
            except (ValueError,TimeoutError,OSError) as exc:
                print(f'diagnostic negotiation failed: {exc}',file=sys.stderr);return 2
        if out:
            class W:pass
            w=W();w.buffer=out;w.flush=out.flush;return transact(fd,a.command,a.seconds,a.max_bytes,w)
        return 0 if transact(fd,a.command,a.seconds,a.max_bytes,sys.stdout)>=0 else 1
    finally:
        if a.all_off_on_exit or a.action in ('leds','calibrate','stress'):
            try:os.write(fd,b'led off\n')
            except OSError:pass
        os.close(fd);out and out.close()
if __name__=='__main__':raise SystemExit(main())
