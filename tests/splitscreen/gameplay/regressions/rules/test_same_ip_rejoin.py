#!/usr/bin/env python3
"""Static and behavioral regression for stale same-IP session counting."""

from __future__ import annotations

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[5]
SOURCE = ROOT / "codemp/game/g_client.c"


def counted_same_ip_peers(
    clients: list[tuple[str, str]], client_num: int, address: str
) -> int:
    """Model the admission count: only other non-disconnected peers count."""
    return sum(
        index != client_num and connected != "CON_DISCONNECTED" and ip == address
        for index, (connected, ip) in enumerate(clients)
    )


class SameIpRejoinRegression(unittest.TestCase):
    def test_stale_disconnected_slot_does_not_reject_fourth_party_member(self) -> None:
        clients = [
            ("CON_CONNECTED", "127.0.0.1"),
            ("CON_CONNECTED", "127.0.0.1"),
            ("CON_CONNECTED", "127.0.0.1"),
            ("CON_DISCONNECTED", "127.0.0.1"),
        ]
        self.assertEqual(counted_same_ip_peers(clients, 3, "127.0.0.1"), 3)
        # Preserve the existing server boundary: rejection is count > limit.
        self.assertFalse(counted_same_ip_peers(clients, 3, "127.0.0.1") > 3)

    def test_other_connected_same_ip_peers_still_count(self) -> None:
        clients = [
            ("CON_CONNECTED", "127.0.0.1"),
            ("CON_CONNECTED", "127.0.0.1"),
            ("CON_CONNECTED", "127.0.0.1"),
            ("CON_CONNECTING", "127.0.0.1"),
            ("CON_DISCONNECTED", "127.0.0.1"),
        ]
        self.assertEqual(counted_same_ip_peers(clients, 4, "127.0.0.1"), 4)
        self.assertTrue(counted_same_ip_peers(clients, 4, "127.0.0.1") > 3)

    def test_source_requires_connected_peer_and_excludes_target_slot(self) -> None:
        text = SOURCE.read_text()
        block = re.search(
            r"if \( i != clientNum &&(?P<body>.*?)CompareIPs"
            r"\( tmpIP, level\.clients\[i\]\.sess\.IP \) \)",
            text,
            re.DOTALL,
        )
        self.assertIsNotNone(block)
        assert block is not None
        self.assertIn(
            "level.clients[i].pers.connected != CON_DISCONNECTED",
            block.group("body"),
        )


if __name__ == "__main__":
    unittest.main()
