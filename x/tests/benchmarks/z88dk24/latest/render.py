import csv, pathlib, sys
source,destination=sys.argv[1:]
rows=list(csv.DictReader(open(source,newline='')))
lanes=['sccz80','xcc_Os','xcc_Of','sdcc','sdcc_max','80cc_fp','80cc_sp','zsdcc','zsdcc_max']
labels=['nightly sccz80','XCC -Os','XCC -Of','official SDCC','official SDCC max','nightly 80cc FP','nightly 80cc SP','nightly zsdcc','nightly zsdcc max']
competitors=[lane for lane in lanes if not lane.startswith('xcc_')]
def valid(row,lane):
 return row[lane+'_status']=='OK' and int(row[lane+'_bytes'])>0 and int(row[lane+'_cycles'])>0
def wins(lane,against,metric):
 pairs=[(int(row[lane+'_'+metric]),min(int(row[c+'_'+metric]) for c in against if valid(row,c))) for row in rows if valid(row,lane) and any(valid(row,c) for c in against)]
 return f"{sum(a<b for a,b in pairs)}/{len(pairs)}"
def cell(row,lane):
 status=row[lane+'_status']
 if status=='SKIP':return '—'
 if status=='BUILD':return 'BUILD FAILED'
 return f"{row[lane+'_bytes']} B / {row[lane+'_cycles']} T" + ('' if status=='OK' else ' '+status)
lines=['# Fresh 2026-09-09 z88dk24 comparison','','All lanes share the fresh nightly headers, +test CRT and classic library. The 24 original workload sources and correctness checks are unchanged. sccz80 and 80cc are built from the downloaded nightly archive matching master 895dc1366573b792d98b0607fb1048d2db248b3d. The latest 80cc source change is e56f2c22b321e55a852c3c159d9cbfdaa3533bac. The nightly bundles zsdcc revision16639; the separate official SDCC trunk lane is revision16858 with the retained z88dk ABI compatibility patch.','','Each cell reports complete linked bytes and measured Z80 T-states. Invalid results remain visible and are excluded from winning comparisons. Both expensive max-allocation probes retain the original six-workload subset.','','| XCC profile | Correct / attempted | Smaller than best valid 80cc | Faster than best valid 80cc | Smaller than every valid competitor | Faster than every valid competitor |','|---|---:|---:|---:|---:|---:|']
for lane in ['xcc_Os','xcc_Of']:
 correct=sum(valid(row,lane) for row in rows);attempted=sum(row[lane+'_status']!='SKIP' for row in rows)
 lines.append('| '+ ' | '.join([lane,f'{correct}/{attempted}',wins(lane,['80cc_fp','80cc_sp'],'bytes'),wins(lane,['80cc_fp','80cc_sp'],'cycles'),wins(lane,competitors,'bytes'),wins(lane,competitors,'cycles')])+' |')
lines+=['','| Benchmark | '+' | '.join(labels)+' |','|---|'+'---:|'*len(lanes)]
for row in rows:lines.append('| '+row['benchmark']+' | '+' | '.join(cell(row,lane) for lane in lanes)+' |')
pathlib.Path(destination).write_text('\n'.join(lines)+'\n')
