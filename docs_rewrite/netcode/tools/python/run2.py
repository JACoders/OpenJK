import bench2 as B
B.run_cfg("gap", variant="gap")
B.run_cfg("flag", variant="flag")
B.run_cfg("gap + value ctx by entity type", variant="gap", valcls=True)
B.run_cfg("gap + lossy quant 1/16 (floats)", variant="gap", quant=16)
