#!/usr/bin/env python3
import csv, hashlib, subprocess, sys, tempfile
from pathlib import Path

VALIDATOR = Path(__file__).resolve().parents[1] / 'scripts' / 'validate_stage21_identity_batch.py'
CONSENT_FIELDS = ['enrollment_images','retained_images','derived_embeddings','pad_captures','export_import_test','deletion_test','automatic_update']

def run(root, *extra):
    return subprocess.run([sys.executable, str(VALIDATOR), str(root), *extra], capture_output=True, text=True)

def main():
    with tempfile.TemporaryDirectory(prefix='dms-stage21-batch-test-') as temp:
        root=Path(temp); consent=[]; manifest=[]
        conditions=[('enrollment',f'enrollment-{n:02d}') for n in range(10)]+[('pad-live','live-01'),('pad-live','live-02'),('pad-attack','print-01'),('pad-attack','replay-01')]
        for number in range(1,6):
            pid=f'ID{number:02d}'; consent_id=f'CONSENT-{number:02d}'; folder=root/pid; folder.mkdir()
            row={'participant_id':pid,'consent_id':consent_id,'consent_date':'2026-09-07'}; row.update({field:'yes' for field in CONSENT_FIELDS}); consent.append(row)
            for index,(kind,condition) in enumerate(conditions,1):
                rel=f'{pid}/{index:02d}.bin'; payload=f'{pid}:{kind}:{condition}'.encode(); (root/rel).write_bytes(payload)
                manifest.append({'participant_id':pid,'capture_id':f'{pid}-C{index:02d}','kind':kind,'condition':condition,'relative_path':rel,'sha256':'','consent_id':consent_id})
        with (root/'consent_register.csv').open('w',encoding='utf-8',newline='') as f:
            w=csv.DictWriter(f,fieldnames=consent[0]);w.writeheader();w.writerows(consent)
        with (root/'capture_manifest.csv').open('w',encoding='utf-8',newline='') as f:
            w=csv.DictWriter(f,fieldnames=manifest[0]);w.writeheader();w.writerows(manifest)
        if run(root).returncode!=2: raise RuntimeError('blank checksums did not produce the expected incomplete result')
        if run(root,'--write-checksums').returncode!=0 or run(root).returncode!=0: raise RuntimeError('valid batch was rejected')
        (root/'ID01'/'01.bin').write_bytes(b'tampered')
        if run(root).returncode!=1: raise RuntimeError('tampered capture was accepted')
        print('stage21 identity batch test PASSED')
    return 0

if __name__=='__main__': raise SystemExit(main())
