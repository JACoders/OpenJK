# CTF/CTY objective lifecycle discovery

This GP2-04 suite runs staged native CTF and CTY objective lifecycles for 2,
3, and 4 local clients on `mp/ctf1`. It records team/identity assertions, enemy
objective pickup, carrier death/drop, teammate return, capture-limit
intermission, restart, and post-restart identity.

The staging uses server-authorized `setviewpos` and `kill` commands, so these
cells are narrow gameplay lifecycle probes, not visible-UI end-to-end passes.
The matrix explicitly keeps UI/device, timed return, remote fifth-player,
disconnect, CTY Force restriction, and next-map coverage unsupported.
