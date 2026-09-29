#!/usr/bin/env python3
"""Normalize a live server-list response into a public-certification gate."""

from __future__ import annotations

import argparse
import base64
import binascii
import json
import re
import sys
from pathlib import Path
from typing import Any, Iterable


TARGET = "135.125.145.49:29070"
ADDRESS_KEYS = ("address", "addr", "endpoint", "serverAddress")
IP_KEYS = ("ip", "host")
PORT_KEYS = ("port", "gamePort")
OCCUPIED_KEYS = (
    "clients",
    "clientCount",
    "clientsCount",
    "numClients",
    "players",
    "playerCount",
    "numPlayers",
)
MAX_KEYS = ("maxClients", "maxclients", "maxPlayers", "maxplayers", "slots")
BOT_KEYS = ("bots", "botCount", "numBots")
HUMAN_KEYS = ("humans", "humanCount", "numHumans")
STATUS_PLAYER = re.compile(r'^\s*(-?\d+)\s+(-?\d+)\s+"')


def dictionaries(value: Any) -> Iterable[dict[str, Any]]:
    if isinstance(value, dict):
        yield value
        for child in value.values():
            yield from dictionaries(child)
    elif isinstance(value, list):
        for child in value:
            yield from dictionaries(child)


def integer(record: dict[str, Any], keys: tuple[str, ...]) -> int | None:
    for key in keys:
        value = record.get(key)
        if isinstance(value, bool):
            continue
        if isinstance(value, (int, float)):
            return int(value)
        if isinstance(value, str) and value.strip().isdigit():
            return int(value)
    return None


def endpoint(record: dict[str, Any]) -> str | None:
    for key in ADDRESS_KEYS:
        value = record.get(key)
        if isinstance(value, str) and ":" in value:
            return value
    ip = next(
        (record.get(key) for key in IP_KEYS if isinstance(record.get(key), str)),
        None,
    )
    if ip and ":" in ip:
        return ip
    port = integer(record, PORT_KEYS)
    return f"{ip}:{port}" if ip and port is not None else None


def player_list(record: dict[str, Any]) -> list[Any] | None:
    for key in ("players", "clients"):
        value = record.get(key)
        if isinstance(value, list):
            return value
    return None


def encoded_status_counts(
    record: dict[str, Any],
) -> tuple[int | None, int | None, int | None, int | None]:
    """Read JK Nexus' live base64 getstatus payload conservatively.

    The live backend exposes the raw Quake 3 status response in ``info`` rather
    than duplicating normalized player-count fields. A strictly positive ping
    is counted as a human. Zero/negative-ping rows still consume server slots
    but are not used to satisfy the nonlocal-human gate.
    """

    encoded = record.get("info")
    if not isinstance(encoded, str) or not encoded:
        return None, None, None, None
    try:
        text = base64.b64decode(encoded, validate=True).decode(
            "latin-1", errors="replace"
        )
    except (ValueError, binascii.Error):
        return None, None, None, None

    lines = text.splitlines()
    info_index = next(
        (
            index
            for index, line in enumerate(lines)
            if line.startswith("\\") and "\\sv_maxclients\\" in line.lower()
        ),
        None,
    )
    if info_index is None:
        return None, None, None, None

    fields = lines[info_index].split("\\")[1:]
    values = {
        fields[index].lower(): fields[index + 1]
        for index in range(0, len(fields) - 1, 2)
    }
    maximum_text = values.get("sv_maxclients", "")
    maximum = int(maximum_text) if maximum_text.isdigit() else None

    pings = [
        int(match.group(2))
        for line in lines[info_index + 1 :]
        if (match := STATUS_PLAYER.match(line))
    ]
    occupied = len(pings)
    humans = sum(ping > 0 for ping in pings)
    bots = occupied - humans
    return occupied, maximum, bots, humans


def counts(record: dict[str, Any]) -> tuple[int | None, int | None, int | None, int | None]:
    listed = player_list(record)
    occupied = len(listed) if listed is not None else integer(record, OCCUPIED_KEYS)
    maximum = integer(record, MAX_KEYS)
    bots = integer(record, BOT_KEYS)
    humans = integer(record, HUMAN_KEYS)
    encoded_occupied, encoded_maximum, encoded_bots, encoded_humans = (
        encoded_status_counts(record)
    )
    if occupied is None:
        occupied = encoded_occupied
    if maximum is None:
        maximum = encoded_maximum
    if bots is None:
        bots = encoded_bots
    if humans is None:
        humans = encoded_humans

    if listed is not None:
        listed_bots = sum(
            1
            for player in listed
            if isinstance(player, dict)
            and (
                player.get("bot") is True
                or player.get("isBot") is True
                or str(player.get("type", "")).lower() == "bot"
            )
        )
        if bots is None:
            bots = listed_bots
        if humans is None:
            humans = len(listed) - listed_bots
    if humans is None and occupied is not None and bots is not None:
        humans = max(occupied - bots, 0)
    return occupied, maximum, bots, humans


def normalize(
    payload: Any, required_slots: int, target: str = TARGET
) -> dict[str, Any]:
    candidates = [record for record in dictionaries(payload) if endpoint(record) == target]
    for record in dictionaries(payload):
        nested = record.get(target)
        if isinstance(nested, dict):
            candidates.append({"address": target, **nested})
    if not candidates:
        return {
            "classification": "BLOCKED_TARGET_NOT_LISTED",
            "target": target,
            "required_slots": required_slots,
        }

    # Prefer the matching object that contains the richest occupancy data.
    record = max(
        candidates,
        key=lambda item: sum(value is not None for value in counts(item)),
    )
    occupied, maximum, bots, humans = counts(record)
    result: dict[str, Any] = {
        "target": target,
        "required_slots": required_slots,
        "occupied": occupied,
        "max_clients": maximum,
        "bots": bots,
        "nonlocal_humans": humans,
    }
    if occupied is None or maximum is None:
        result["classification"] = "BLOCKED_CAPACITY_UNKNOWN"
        return result

    free_slots = maximum - occupied
    result["free_slots"] = free_slots
    if humans is None:
        result["classification"] = "BLOCKED_HUMAN_OCCUPANCY_UNKNOWN"
    elif humans < 1:
        result["classification"] = "BLOCKED_NO_NONLOCAL_HUMAN"
    elif free_slots < required_slots:
        result["classification"] = "BLOCKED_INSUFFICIENT_CAPACITY"
    else:
        result["classification"] = "READY"
    return result


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("response", type=Path)
    parser.add_argument("required_slots", type=int, choices=(2, 3, 4))
    parser.add_argument("--output", type=Path)
    parser.add_argument("--target", default=TARGET)
    args = parser.parse_args()
    try:
        payload = json.loads(args.response.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as exc:
        print(f"invalid server-list response: {exc}", file=sys.stderr)
        return 2

    result = normalize(payload, args.required_slots, args.target)
    rendered = json.dumps(result, indent=2, sort_keys=True) + "\n"
    if args.output:
        args.output.write_text(rendered, encoding="utf-8")
    else:
        sys.stdout.write(rendered)
    return 0 if result["classification"] == "READY" else 78


if __name__ == "__main__":
    raise SystemExit(main())
