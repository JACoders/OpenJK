# Hosting And Network Play

## Local Server

Choose **Local Match**, finish all player profiles, and apply. The build opens
the stock Create Server workflow, including the normal map, game type, rules,
bots, password, dedicated, and visibility controls. Starting the server attaches
the remaining local clients to `localhost` after Player 1 enters the match.

Supported stock modes include Free For All, Holocron, Jedi Master, Duel, Power
Duel, Team FFA, Siege, Capture the Flag, and Capture the Ysalamiri. Mode-specific
team, duel, class, round, respawn, and score rules are still owned by the server.

To allow a fifth player from another machine:

- Set enough public/private slots for all local and remote clients.
- Allow at least four connections from the host IP when the server exposes a
  per-IP connection limit.
- For LAN play, the remote player joins the host's LAN address.
- For internet hosting, allow the configured UDP game port through the host
  firewall and router/NAT. The stock default is UDP 29070.

## Existing Server

Choose **Server Party**, finish profiles, and apply. The stock server browser
opens with the party pending. Select one server once; Players 2 through 4 make
their own ordinary protocol connections to the same address.

The implementation has been tested against an untouched upstream OpenJK
dedicated server and with an untouched fifth OpenJK client. It adds no required
protocol extension and no server-side mod requirement. The optional unknown
`splitplayer` userinfo key is legal Quake 3 userinfo and ignored by vanilla
servers.

## Server Restrictions

All local players are separate server slots and share one public IP. A server can
still reject party members because it is full, passworded, has a low per-IP
limit, requires a mod/download, bans a user, or enforces authentication. Each
rejection is tracked for that player; the other connected viewports remain
active.

All players in one process must connect to the same server. Renderer, window,
audio device, filesystem, and server-browser selection are process-wide. Names,
models, sabers, Force powers, teams, bindings, gameplay commands, consoles, and
archived split-player settings are per player.
