#!/usr/bin/env python3
import argparse, csv, hashlib
from collections import Counter
from pathlib import Path

YES = {'yes', 'y', 'true', '1'}
CONSENT_FIELDS = ['enrollment_images','retained_images','derived_embeddings','pad_captures','export_import_test','deletion_test','automatic_update']

def rows(path):
    with path.open('r', encoding='utf-8-sig', newline='') as stream:
        return list(csv.DictReader(stream))

def main():
    parser=argparse.ArgumentParser(description='Validate a private five-person Stage 21 identity/PAD batch without modifying it.')
    parser.add_argument('root', type=Path)
    parser.add_argument('--write-checksums', action='store_true', help='Populate missing SHA-256 fields after all other checks pass.')
    args=parser.parse_args(); root=args.root.resolve(); errors=[]
    consent_path=root/'consent_register.csv'; manifest_path=root/'capture_manifest.csv'
    if not consent_path.is_file() or not manifest_path.is_file(): raise SystemExit('missing consent_register.csv or capture_manifest.csv')
    consent=rows(consent_path); manifest=rows(manifest_path)
    consent_by_id={r.get('participant_id',''):r for r in consent}
    if len(consent_by_id)!=5 or len(consent)!=5: errors.append('consent register must contain five unique anonymous participants')
    for pid,r in consent_by_id.items():
        if not pid or not r.get('consent_id') or not r.get('consent_date'): errors.append(f'{pid or "<blank>"}: consent ID/date incomplete')
        for field in CONSENT_FIELDS:
            if r.get(field,'').strip().lower() not in YES: errors.append(f'{pid}: {field} not consented')
    seen=set(); counts=Counter(); pending=[]
    for line,r in enumerate(manifest,2):
        pid=r.get('participant_id',''); cid=r.get('capture_id',''); kind=r.get('kind','')
        if cid in seen: errors.append(f'line {line}: duplicate capture_id {cid}')
        seen.add(cid); counts[(pid,kind)]+=1
        c=consent_by_id.get(pid)
        if not c: errors.append(f'line {line}: unknown participant {pid}')
        elif r.get('consent_id') != c.get('consent_id'): errors.append(f'line {line}: consent_id mismatch')
        rel=r.get('relative_path',''); path=(root/rel).resolve() if rel else None
        if not rel or not path or not path.is_file() or root not in path.parents: errors.append(f'line {line}: missing/unsafe capture path'); continue
        digest=hashlib.sha256(path.read_bytes()).hexdigest()
        recorded=r.get('sha256','').lower()
        if recorded and recorded!=digest: errors.append(f'line {line}: checksum mismatch')
        elif not recorded: pending.append((r,digest))
    for pid in consent_by_id:
        if counts[(pid,'enrollment')]!=10: errors.append(f'{pid}: expected 10 enrollment captures')
        if counts[(pid,'pad-live')]<2: errors.append(f'{pid}: expected at least 2 live PAD captures')
        if counts[(pid,'pad-attack')]<2: errors.append(f'{pid}: expected at least 2 consented attack PAD captures')
    if errors:
        print('\n'.join('ERROR: '+e for e in errors)); return 1
    if pending and args.write_checksums:
        for r,digest in pending:r['sha256']=digest
        with manifest_path.open('w',encoding='utf-8',newline='') as stream:
            writer=csv.DictWriter(stream,fieldnames=manifest[0].keys());writer.writeheader();writer.writerows(manifest)
        pending=[]
    if pending: print(f'VALID EXCEPT CHECKSUMS: {len(pending)} SHA-256 values are blank; rerun with --write-checksums'); return 2
    print(f'VALID: {len(consent_by_id)} participants, {len(manifest)} captures, consent and checksums complete'); return 0

if __name__=='__main__': raise SystemExit(main())
