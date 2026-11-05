import bench2 as B, collections
from collections import Counter
print('actual delta values in 5min:', Counter(s['delta'] for s in B.data['5min']).most_common(6))
for k in (1,2,4,8):
    B.run_cfg("base age k=%d snapshots"%k, k=k, variant="gap", sets=("5min-test","bots"))
