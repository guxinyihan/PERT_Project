"""Auxiliary test runner only. The delivered application is entirely C11.
Run from the project root: python tests/automated_tests.py
Requires Python 3.8+ and GCC on PATH. No third-party Python packages.
"""
import json
import math
import os
from pathlib import Path
import random
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
os.chdir(ROOT)
RESULTS = ROOT / 'tests' / 'results'
RESULTS.mkdir(exist_ok=True)
CORE = ['src/' + n + '.c' for n in ['model','input','graph','pert','probability','output']]
FLAGS = ['-std=c11','-Wall','-Wextra','-Wpedantic','-Werror','-O2','-Iinclude']
records = []

def command(args, log):
    p = subprocess.run(args, capture_output=True, text=True, timeout=180)
    (RESULTS / log).write_text('$ ' + ' '.join(args) + '\n' + p.stdout + p.stderr, encoding='utf-8')
    if p.returncode:
        raise AssertionError(f'{log}: exit {p.returncode}\n{p.stdout}\n{p.stderr}')
    return p.stdout

def passed(name, detail=''):
    records.append({'name':name, 'status':'PASS', 'detail':detail})
    print('PASS', name, detail, flush=True)

def cli(text):
    p = subprocess.run([str(ROOT/'pert.exe')], input=text, capture_output=True, text=True, timeout=30)
    assert p.returncode == 0, p.stderr
    return p.stdout + p.stderr

def csv_cli(body, deadline='47', raw=False):
    with tempfile.TemporaryDirectory(prefix='pert_test_') as td:
        f=Path(td)/'case.csv'
        f.write_bytes(body if raw else ('ID,Description,a,m,b,Predecessors\n'+body).encode())
        return cli(f'1\n{f}\n{deadline}\n0\n')

command(['gcc', *FLAGS, 'src/main.c', *CORE, '-o','pert.exe','-lm'], 'build.txt')
command(['gcc', *FLAGS, 'tests/test_core.c', *CORE,'-o','tests/test_core.exe','-lm'], 'build_core.txt')
passed('Warning-free GCC C11 builds', 'Includes -Werror')
out=command([str(ROOT/'tests/test_core.exe')], 'core.txt')
assert 'ALL CORE TESTS PASSED' in out
passed('C core regression suite', out.strip().splitlines()[-1])
command(['gcc', *FLAGS, '-DTRACK_MEMORY','-include','tests/memory_redirect.h',
         'tests/test_core.c','tests/memory_probe.c', *CORE,
         '-o','tests/test_memory.exe','-lm'], 'build_memory.txt')
out=command([str(ROOT/'tests/test_memory.exe')], 'memory.txt')
assert 'PASS allocation tracker' in out
passed('Allocation tracking and exhaustive lecture allocation failure sweep', out.strip().splitlines()[-1])
asan=subprocess.run(['gcc',*FLAGS,'-fsanitize=address','src/main.c',*CORE,'-o','tests/test_asan.exe','-lm'],capture_output=True,text=True)
(RESULTS/'asan.txt').write_text(asan.stdout+asan.stderr,encoding='utf-8')
if asan.returncode:
    records.append({'name':'AddressSanitizer','status':'UNAVAILABLE','detail':asan.stderr.strip()})
else:
    p=subprocess.run([str(ROOT/'tests/test_asan.exe')], input='1\ndata/lecture_example.csv\n47\n0\n',capture_output=True,text=True)
    assert p.returncode==0 and 'AddressSanitizer' not in p.stderr
    passed('AddressSanitizer classroom run')

out=cli('1\ndata/lecture_example.csv\n47\n0\n')
(RESULTS/'lecture_output.txt').write_text(out,encoding='utf-8')
assert 'P=84.1345%' in out and 'A -> B -> C -> E -> F -> J -> L -> N' in out
passed('Classroom CLI output', '44 days; variance 9; sigma 3; Z 1; 84.1345%')
out=cli('9\n1\ndata/lecture_example.csv\n-1\nNaN\n47\n1\ndata/additional_example_files.csv\n6\n0\n')
assert out.count('1. Project Overview')==2 and out.count('Invalid deadline:')==2
assert 'Emitted critical paths: 2' in out
passed('Repeated menu analysis and invalid deadline recovery')
out=cli('2\n0\n2\n1bad\nA\n\nFirst\n-1\n1\n0\n1\n1\nA\nB\nSecond\n2\n2\n2\n-\nUnknown\nA|A\nA\n3\n0\n')
(RESULTS/'keyboard_output.txt').write_text(out,encoding='utf-8')
assert 'Project expected duration: 3 days' in out and 'Emitted critical paths: 1' in out
assert 'Duplicate dependency' in out
passed('Keyboard field re-entry and predecessor rollback')
out=cli('2\n2\nA\nOne\n1\n1\n1\nB\nTwo\n1\n1\n1\nB\nA\n0\n')
assert 'CYCLE' in out and 'Target deadline' not in out
passed('Keyboard cycle recovery before deadline')
out=cli('2\n1\nA\n')
assert 'cancelled' in out and 'Goodbye' in out
passed('EOF during keyboard input')

cases=[
 ('UTF-8 BOM CRLF whitespace and empty lines', b'\xef\xbb\xbf\r\n ID , Description , a , m , b , Predecessors \r\n\r\n B , later , 1 , 1 , 1 , A \r\n A , first , 1 , 1 , 1 , - \r\n', True),
 ('Wrong header', b'ID,Description,A,m,b,Predecessors\nA,One,1,1,1,-\n',False),
 ('Embedded NUL', b'ID,Description,a,m,b,Predecessors\nA,O\x00ne,1,1,1,-\n',False),
 ('Quoted comma unsupported', b'ID,Description,a,m,b,Predecessors\nA,"one,two",1,1,1,-\n',False),
 ('Maximum 256-byte description', ('ID,Description,a,m,b,Predecessors\nA,'+'x'*256+',1,1,1,-\n').encode(),True),
 ('Description over 256 bytes', ('ID,Description,a,m,b,Predecessors\nA,'+'x'*257+',1,1,1,-\n').encode(),False),
 ('Long line no silent truncation', ('ID,Description,a,m,b,Predecessors\nA,'+'x'*20000+',1,1,1,-\n').encode(),False),
 ('Maximum 32-byte ID', ('ID,Description,a,m,b,Predecessors\n'+'A'*32+',one,1,1,1,-\n').encode(),True),
 ('ID over 32 bytes', ('ID,Description,a,m,b,Predecessors\n'+'A'*33+',one,1,1,1,-\n').encode(),False),
 ('Case-sensitive IDs', b'ID,Description,a,m,b,Predecessors\nA,one,1,1,1,-\na,two,1,1,1,A\n',True),
 ('Missing header', b'',False),
]
for name,data,valid in cases:
    out=csv_cli(data,raw=True)
    assert ('1. Project Overview' in out)==valid, (name,out)
    if not valid:
        assert 'Error [' in out and 'Target deadline' not in out, out
        assert 'case.csv' in out
    passed(name)
out=csv_cli('A,one,1,1,1,-\nB,two,1,1,1,Z\n')
assert 'line 3, activity B' in out and "Predecessor 'Z'" in out
passed('Filename line number and activity diagnostics')
out=csv_cli('A,Long,1,1,1,-\nB,Short,0.99999998,0.99999998,0.99999998,-\n')
row=next(x for x in out.splitlines() if x.startswith('B | 1.0000 |'))
assert row.endswith('NO') and float(row.split('|')[-2])>1e-9
passed('Small nonzero slack remains visible and noncritical')

# Independent DAG oracle: positive integer deterministic times avoid ambiguous
# near-zero comparisons; input order shuffled to test ID resolution and Kahn.
rng=random.Random(20260918)
for trial in range(100):
    n=rng.randint(2,18); duration=[rng.randint(1,9) for _ in range(n)]
    pred=[[j for j in range(i) if rng.random()<0.17] for i in range(n)]
    succ=[[] for _ in range(n)]
    for i,ps in enumerate(pred):
        for j in ps: succ[j].append(i)
    es=[]; ef=[]
    for i in range(n):
        es.append(max([ef[j] for j in pred[i]], default=0)); ef.append(es[i]+duration[i])
    total=max(ef); ls=[0]*n; lf=[0]*n
    for i in reversed(range(n)):
        lf[i]=min([ls[j] for j in succ[i]],default=total); ls[i]=lf[i]-duration[i]
    order=list(range(n)); rng.shuffle(order)
    body=''.join(f'A{i},Node,{duration[i]},{duration[i]},{duration[i]},'+('|'.join(f'A{j}' for j in pred[i]) or '-')+'\n' for i in order)
    out=csv_cli(body,str(total)); block=out.split('7. Final PERT Results\n')[1].split('\n8.')[0]
    rows=[x.split('|') for x in block.splitlines() if re.match(r'A\d+ \|',x)]
    assert len(rows)==n
    for row in rows:
        i=int(row[0].strip()[1:]); actual=list(map(float,row[1:8]))
        assert actual==[duration[i],0,es[i],ef[i],ls[i],lf[i],ls[i]-es[i]]
        assert (row[8].strip()=='YES')==(ls[i]==es[i])
    emitted=re.findall(r'^Path \d+: (.+)$',out,re.M)
    for path in emitted:
        seq=[int(x[1:]) for x in path.split(' -> ')]
        assert not pred[seq[0]] and not succ[seq[-1]]
        assert sum(duration[x] for x in seq)==total
        assert all(a in pred[b] for a,b in zip(seq,seq[1:]))
passed('Independent randomized scheduling oracle', '100 shuffled DAGs; every result row and emitted path checked')
(RESULTS/'summary.json').write_text(json.dumps(records,indent=2),encoding='utf-8')
print(f'COMPLETE: {sum(r["status"]=="PASS" for r in records)} checks passed; '+
      f'{sum(r["status"]=="UNAVAILABLE" for r in records)} unavailable checker(s).')
