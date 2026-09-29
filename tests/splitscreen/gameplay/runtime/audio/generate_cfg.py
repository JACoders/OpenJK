#!/usr/bin/env python3
from pathlib import Path

HERE = Path(__file__).resolve().parent
OUT = HERE / "cfg"
OUT.mkdir(parents=True, exist_ok=True)

for players in (2, 3, 4):
    lines = [
        "set cl_splitScreen 1", f"set ui_splitScreenPlayerCount {players}",
        "set cl_splitScreenLocalCmds 0", "set s_initsound 1",
        "set s_volume 0.8", "set s_musicvolume 0.2", "set g_gametype 0",
        "set fraglimit 10", "set sv_maxclients 8", "set g_maxConnPerIP 8",
        "devmap mp/ffa3", "wait 720", "closemenu", "cmd team free",
    ]
    lines += [f"splitnet_connect {p} localhost" for p in range(2, players + 1)]
    lines += ["wait 480"]
    lines += [f"splitnet_cmd {p} team free" for p in range(2, players + 1)]
    lines += ["wait 240", "echo GP305:ATTACHED"]
    lines += [f"splitnet_assert_lifecycle {p} ALIVE" for p in range(1, players + 1)]
    lines += ["screenshot_png gp305_audio_attached", "echo GP305:PER-PANE-ACTIONS",
              "splitinput_device_key keyboard MOUSE1 1", "wait 30",
              "splitinput_assert_cmd 1 -999 -999 -999 1",
              "splitinput_device_key keyboard MOUSE1 0"]
    for p in range(2, players + 1):
        lines += [f"splitinput_device_button controller{p-1} 0 1", "wait 30",
                  f"splitinput_assert_cmd {p} -999 -999 -999 1",
                  f"splitinput_device_button controller{p-1} 0 0"]
    lines += ["say GP305_network_chat", f"splitnet_cmd {players} kill", "wait 180",
              f"splitnet_assert_lifecycle {players} DEAD",
              f"splitinput_device_button controller{players-1} 0 1", "wait 30",
              f"splitinput_device_button controller{players-1} 0 0", "wait 300",
              f"splitnet_assert_lifecycle {players} ALIVE",
              "echo GP305:SND-RESTART", "snd_restart", "wait 600",
              "echo GP305:SND-RESTART-COMPLETE"]
    lines += [f"splitnet_assert_lifecycle {p} ALIVE" for p in range(1, players + 1)]
    lines += ["map_restart 0", "wait 600", "echo GP305:MAP-RESTART-COMPLETE"]
    lines += [f"splitnet_assert_lifecycle {p} ALIVE" for p in range(1, players + 1)]
    lines += ["screenshot_png gp305_audio_restarted", "wait 900",
              "echo GP305:AUDIO-SHUTDOWN", "quit"]
    (OUT / f"audio_{players}p.cfg").write_text("\n".join(lines) + "\n")

(OUT / "remote.cfg").write_text("""\
set cl_splitScreen 0
set s_initsound 1
set name GP305_Remote
wait 900
vstr gp305_connect
wait 1200
cmd team free
say GP305_remote_voice_text
wait 600
quit
""")
