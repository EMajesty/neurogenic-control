#!/usr/bin/env python3
"""Verify the checked manifest against resolved KiCad PCB netlists."""
import argparse, json, pathlib, re, sys
ROOT = pathlib.Path(__file__).resolve().parents[1]

def blocks(text, tag):
    for m in re.finditer(r"\(" + re.escape(tag) + r"\s", text):
        start=m.start(); depth=0; quote=False; escape=False
        for i in range(start, len(text)):
            c=text[i]
            if quote:
                if escape: escape=False
                elif c=="\\": escape=True
                elif c=='"': quote=False
            elif c=='"': quote=True
            elif c=='(': depth += 1
            elif c==')':
                depth -= 1
                if depth == 0: yield text[start:i+1]; break

def pcb_components(board):
    text=(ROOT/f"hardware/{board}/{board}.kicad_pcb").read_text()
    out={}
    for fp in blocks(text,"footprint"):
        rm=re.search(r'\(property "Reference" "([^"]+)"',fp)
        vm=re.search(r'\(property "Value" "([^"]+)"',fp)
        if not rm: continue
        pads={}
        for pad in blocks(fp,"pad"):
            pm=re.match(r'\(pad "([^"]*)"',pad); nm=re.search(r'\(net \d+ "([^"]+)"\)',pad)
            if pm and nm: pads[pm.group(1)]=nm.group(1)
        out[rm.group(1)]={"value":vm.group(1) if vm else "","pads":pads}
    return out

def check(cond,msg,errors):
    if not cond: errors.append(msg)

def schematic_components(path):
    text=path.read_text(); out={}
    for net in blocks(text,'net'):
        name=re.search(r'\(name "([^"]+)"\)',net)
        if not name: continue
        for node in blocks(net,'node'):
            ref=re.search(r'\(ref "([^"]+)"\)',node); pin=re.search(r'\(pin "([^"]+)"\)',node)
            if ref and pin: out.setdefault(ref.group(1),{})[pin.group(1)]=name.group(1)
    return out

def main():
    ap=argparse.ArgumentParser();ap.add_argument('--schematic-dir',type=pathlib.Path);args=ap.parse_args()
    manifest=json.loads((ROOT/'hardware/manifest.json').read_text()); errors=[]
    pcbs={b:pcb_components(b) for b in ('brain','buttons-1x8','buttons-2x5','encoders','nav','pads')}
    pico=pcbs['brain']['A1']['pads']; pico_gpio={0:1,1:2,2:4,3:5,4:6,5:7,6:9,7:10,8:11,9:12,10:14,11:15,12:16,13:17,14:19,15:20,16:21,17:22,18:24,19:25,20:26,21:27,22:29,26:31,27:32,28:34}
    for net,gpio in manifest['gpio'].items():
        expected={'RGB_DATA_RAW':'Net-(A1-GPIO22)','SCAN_CLK':'Net-(A1-GPIO14)','SCAN_LOAD':'Net-(A1-GPIO15)'}.get(net,net)
        check(pico.get(str(pico_gpio[gpio]))==expected,f"GPIO{gpio} {net}: PCB has {pico.get(str(pico_gpio[gpio]))}",errors)
    pin_for_bit=[6,5,4,3,14,13,12,11] # first serial bit through last, QH first
    for board,spec in manifest['board_types'].items():
        for reg in spec.get('registers',[]):
            pads=pcbs[board][reg['ref']]['pads']
            actual=[pads.get(str(pin)) for pin in pin_for_bit]
            declared=reg['bits']
            resolved=[]
            for net in actual:
                if net and ('+3.3V' in net or 'unconnected' in net): resolved.append('NC'); continue
                name=net
                for ref,component in pcbs[board].items():
                    if board=='encoders': continue
                    if not ref.startswith('SW'): continue
                    for pin,pnet in component['pads'].items():
                        if pnet==net and pnet!='GND':
                            name=(f"DATA.{pin}" if board=='nav' and ref=='SW1' else ref)
                resolved.append(name)
            actual=resolved
            check(actual==declared,f"{board}.{reg['ref']} bits: declared {declared}, PCB {actual}",errors)
            check(pads.get('1')=='SCAN_LOAD' and pads.get('2')=='SCAN_CLK',f"{board}.{reg['ref']} clock/load mismatch",errors)
    tlc=pcbs['encoders']['U3']['pads']; declared=manifest['board_types']['encoders']['tlc5947']['outputs']
    check([tlc.get(str(i)) for i in range(5,29)]==declared,"TLC5947 output mapping mismatch",errors)
    check(tlc.get('1')=='GND' and tlc.get('32')=='+3.3V' and tlc.get('3')=='SCAN_CLK',"TLC5947 power/clock mismatch",errors)
    for b in ('buttons-1x8','buttons-2x5','nav','pads'):
        j=pcbs[b]['J1']['pads']
        expected={'1':'GND','2':'+3.3V','3':'SCAN_CLK','4':'SCAN_LOAD','5':'BUTTON_DATA','6':'DIN','9':'GND','10':'+5V'}
        check(all(j.get(pin)==net for pin,net in expected.items()),f"{b}.J1 connector mismatch",errors)
    for port in manifest['brain_ports']:
        j=pcbs['brain'][port['id'].split('.')[1]]['pads']
        check(j.get('1')=='GND' and j.get('2')=='+3.3V' and j.get('3')=='SCAN_CLK' and j.get('4')=='SCAN_LOAD' and j.get('5')==port['data'] and j.get('9')=='GND' and j.get('10')=='+5V',f"{port['id']} connector mismatch",errors)
    check(pcbs['brain']['U2']['pads']=={'1':'+12V','2':'GND','3':'+5V'},"brain.U2 regulator rails mismatch",errors)
    for b in ('buttons-1x8','buttons-2x5','nav'):
        order=manifest['board_types'][b]['rgb_order']; comps=pcbs[b]
        check(all(x in comps for x in order),f"{b} RGB member missing",errors)
    ids=[]
    for board,spec in manifest['board_types'].items():
        for reg in spec.get('registers',[]): ids += [f"{board}:{reg['ref']}:{x}" for x in reg['bits'] if x!='NC']
    check(len(ids)==len(set(ids)),"duplicate qualified physical input",errors)
    check(manifest['assembly']['sk6812_count']==2*8+2*10+6+16,"SK6812 inventory arithmetic mismatch",errors)
    if args.schematic_dir:
        for board,pcb in pcbs.items():
            sch=schematic_components(args.schematic_dir/f'{board}.net')
            for ref in [r for r in pcb if r.startswith(('A','J','U','SW','LED'))]:
                if ref not in sch: continue
                for pin,net in pcb[ref]['pads'].items():
                    if pin in sch[ref]: check(sch[ref][pin]==net,f'{board}.{ref}.{pin}: schematic {sch[ref][pin]} != PCB {net}',errors)
    if errors:
        print("MANIFEST VERIFICATION FAILED",file=sys.stderr)
        for e in errors: print(f"- {e}",file=sys.stderr)
        return 1
    print("manifest: 6 PCB netlists verified; GPIO, chains, inputs, TLC outputs, power/connector invariants OK")
    print("warning: assembly.port_assignment remains UNVERIFIED_HARNESS_ASSIGNMENT")
    if args.schematic_dir: print("schematic: fresh exported component pin nets agree with resolved PCB nets")
    return 0
if __name__=='__main__': raise SystemExit(main())
