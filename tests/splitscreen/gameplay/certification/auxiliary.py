#!/usr/bin/env python3
"""Fail-closed validation helpers for same-suite certification probes."""

from __future__ import annotations

from hashlib import sha256
import json
from pathlib import Path
import re


def digest(path: Path) -> str:
    return sha256(path.read_bytes()).hexdigest()


def fields(path: Path) -> dict[str, list[list[str]]]:
    parsed: dict[str, list[list[str]]] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        parts = line.split("\t")
        if len(parts) >= 2:
            parsed.setdefault(parts[0], []).append(parts[1:])
    return parsed


def validate_bundle(
    index: Path,
    ledger_sha: str,
    *,
    run_id: str,
    kind: str,
    players: int,
    expected_sha: str,
    started_epoch: int,
) -> tuple[bool, list[str], dict, Path | None]:
    """Validate an owned auxiliary index, its E2E manifest, and report."""

    errors: list[str] = []
    if not index.is_file():
        return False, [f"{kind}: missing auxiliary index"], {}, None
    if digest(index) != ledger_sha:
        errors.append(f"{kind}: auxiliary ledger hash mismatch")
    if int(index.stat().st_mtime) < started_epoch:
        errors.append(f"{kind}: auxiliary index predates suite")
    data = fields(index)
    if data.get("format") != [["openjk-cert-aux-v1"]]:
        errors.append(f"{kind}: invalid auxiliary format")
    if data.get("run_id") != [[run_id]]:
        errors.append(f"{kind}: auxiliary run_id mismatch")
    if data.get("kind") != [[kind]]:
        errors.append(f"{kind}: auxiliary kind mismatch")
    if data.get("players") != [[str(players)]]:
        errors.append(f"{kind}: auxiliary player count mismatch")
    if data.get("frozen_sha256") != [[expected_sha]]:
        errors.append(f"{kind}: auxiliary frozen hash mismatch")

    artifact_rows = data.get("artifact", [])
    artifacts = {
        row[0]: row[1] for row in artifact_rows if len(row) == 2
    }
    if len(artifacts) != len(artifact_rows) or not artifacts:
        errors.append(f"{kind}: malformed or empty auxiliary artifact set")
    for artifact_text, recorded_sha in artifacts.items():
        artifact = Path(artifact_text)
        if not artifact.is_file() or not artifact.stat().st_size:
            errors.append(f"{kind}: missing auxiliary artifact: {artifact}")
        elif digest(artifact) != recorded_sha:
            errors.append(f"{kind}: auxiliary artifact hash mismatch: {artifact}")

    manifest_rows = data.get("manifest", [])
    report_rows = data.get("report", [])
    if len(manifest_rows) != 1 or len(manifest_rows[0]) != 2:
        errors.append(f"{kind}: expected one hash-recorded E2E manifest")
        manifest = None
    else:
        manifest = Path(manifest_rows[0][0])
        if (
            not manifest.is_file()
            or digest(manifest) != manifest_rows[0][1]
            or artifacts.get(str(manifest)) != manifest_rows[0][1]
        ):
            errors.append(f"{kind}: E2E manifest hash mismatch")
            manifest = None
    if len(report_rows) != 1 or len(report_rows[0]) != 2:
        errors.append(f"{kind}: expected one hash-recorded semantic report")
        report_path = None
    else:
        report_path = Path(report_rows[0][0])
        if (
            not report_path.is_file()
            or digest(report_path) != report_rows[0][1]
            or artifacts.get(str(report_path)) != report_rows[0][1]
        ):
            errors.append(f"{kind}: semantic report hash mismatch")
            report_path = None

    client_log: Path | None = None
    if manifest is not None:
        manifest_data = fields(manifest)
        if manifest_data.get("format") != [["openjk-e2e-v1"]]:
            errors.append(f"{kind}: invalid E2E manifest format")
        if manifest_data.get("status") != [["passed"]]:
            errors.append(f"{kind}: E2E manifest status is not passed")
        if manifest_data.get("players") != [[str(players)]]:
            errors.append(f"{kind}: E2E manifest player count mismatch")
        if not any(
            len(row) == 2 and row[1] == expected_sha
            for row in manifest_data.get("hash", [])
        ):
            errors.append(f"{kind}: frozen executable hash absent from E2E manifest")
        log_rows = manifest_data.get("process.client.log", [])
        if len(log_rows) != 1 or len(log_rows[0]) != 1:
            errors.append(f"{kind}: authoritative client log absent")
        else:
            client_log = Path(log_rows[0][0])
            manifest_artifacts = {
                row[0]: row[1]
                for row in manifest_data.get("artifact", [])
                if len(row) == 2
            }
            if (
                not client_log.is_file()
                or manifest_artifacts.get(str(client_log)) != digest(client_log)
                or artifacts.get(str(client_log)) != digest(client_log)
            ):
                errors.append(f"{kind}: authoritative client log is not hash-verified")

    report: dict = {}
    if report_path is not None:
        try:
            parsed = json.loads(report_path.read_text(encoding="utf-8"))
            if not isinstance(parsed, dict):
                raise ValueError("report root is not an object")
            report = parsed
        except (OSError, ValueError, json.JSONDecodeError) as exc:
            errors.append(f"{kind}: invalid semantic report: {exc}")
    return not errors, errors, report, client_log


AUXILIARY_KINDS = (
    "modal_ownership",
    "virtual_controller_restart",
    "asymmetric_network_recovery",
)


def validate_semantics(
    kind: str, report: dict, client_log: Path | None, *, players: int,
    run_id: str, expected_sha: str,
) -> list[str]:
    errors: list[str] = []
    text = client_log.read_text(
        encoding="utf-8", errors="replace"
    ) if client_log is not None and client_log.is_file() else ""
    if kind == "modal_ownership":
        required = {(2, "top"), (2, "score"), (2, "console"), (2, "chat")}
        if players == 3:
            required |= {(3, "top"), (3, "score")}
        cases = report.get("cases")
        actual = {
            (case.get("player"), case.get("surface"))
            for case in cases
            if isinstance(case, dict)
        } if isinstance(cases, list) else set()
        if report.get("schema_version") != 2 or report.get("players") != players:
            errors.append(f"{kind}: semantic report identity mismatch")
        if report.get("passed") is not True or actual != required:
            errors.append(f"{kind}: required pane/surface matrix did not pass exactly")
        elif any(
            case.get("passed") is not True
            or case.get("nonowner_bleed") is not False
            for case in cases
        ):
            errors.append(f"{kind}: owner presence or non-owner isolation failed")
    elif kind == "virtual_controller_restart":
        if (
            report.get("schema_version") != 1
            or report.get("kind") != kind
            or report.get("players") != players
            or report.get("run_id") != run_id
            or report.get("frozen_sha256") != expected_sha
            or report.get("passed") is not True
        ):
            errors.append(f"{kind}: semantic report did not pass for this suite")
        minimum = (players - 1) * 4
        if report.get("input_assert_passes", 0) < minimum:
            errors.append(f"{kind}: insufficient positive input assertions")
        if report.get("screenshot_present") is not True:
            errors.append(f"{kind}: screenshot proof is missing")
        if (
            report.get("physical_hotplug_covered") is not False
            or report.get("physical_hotplug_status") != "BLOCKED_UNSUPPORTED"
        ):
            errors.append(f"{kind}: physical-hotplug limitation was obscured")
        for marker in (
            "CERT-CONTROLLER:PRE-READY",
            "CERT-CONTROLLER:PRE-CHECKED",
            "CERT-CONTROLLER:POST-READY",
            "CERT-CONTROLLER:POST-CHECKED",
            "CERT-CONTROLLER:COMPLETE",
        ):
            if marker not in text:
                errors.append(f"{kind}: missing marker: {marker}")
    elif kind == "asymmetric_network_recovery":
        if (
            report.get("schema_version") != 1
            or report.get("kind") != kind
            or report.get("players") != players
            or report.get("run_id") != run_id
            or report.get("frozen_sha256") != expected_sha
            or report.get("passed") is not True
        ):
            errors.append(f"{kind}: semantic report did not pass for this suite")
        if report.get("phases") != {
            "BASELINE": True, "DISCONNECT": True, "REJOIN": True
        }:
            errors.append(f"{kind}: ordered phase proof is incomplete")
        if (
            report.get("healthy_identity_preserved") is not True
            or report.get("ordered_rejoin") is not True
            or report.get("screenshots_present") is not True
        ):
            errors.append(f"{kind}: identity/rejoin/screenshot proof is incomplete")
        if (
            report.get("whole_party_reconnect_covered") is not False
            or report.get("server_loss_recovery_covered") is not False
        ):
            errors.append(f"{kind}: unsupported broader recovery was overstated")
        for marker in (
            "CERT-ASYMMETRIC:BASELINE-BEGIN",
            "CERT-ASYMMETRIC:DISCONNECT-BEGIN",
            "CERT-ASYMMETRIC:REJOIN-BEGIN",
            "CERT-ASYMMETRIC:COMPLETE",
        ):
            if marker not in text:
                errors.append(f"{kind}: missing marker: {marker}")
    else:
        errors.append(f"unknown auxiliary kind: {kind}")
    if re.search(r"Split[A-Za-z0-9_]*Assert[A-Za-z0-9_]*: FAIL|Sys_Error", text):
        errors.append(f"{kind}: authoritative client log contains a failure")
    return errors


def validate_auxiliary_rows(
    rows: list[list[str]], *, run_id: str, players: int, expected_sha: str,
    started_epoch: int,
) -> tuple[dict[str, bool], dict[str, list[str]]]:
    checks: dict[str, bool] = {}
    all_errors: dict[str, list[str]] = {}
    relevant = [
        row for row in rows if row and row[0] in AUXILIARY_KINDS
    ]
    by_kind = {
        row[0]: row[1:] for row in relevant if len(row) == 3
    }
    exact = len(relevant) == len(AUXILIARY_KINDS) and set(by_kind) == set(
        AUXILIARY_KINDS
    )
    checks["auxiliary_ledger_exact"] = exact
    for kind in AUXILIARY_KINDS:
        row = by_kind.get(kind)
        if row is None:
            checks[kind] = False
            all_errors[kind] = ["missing current-suite auxiliary ledger row"]
            continue
        ok, errors, report, client_log = validate_bundle(
            Path(row[0]), row[1], run_id=run_id, kind=kind, players=players,
            expected_sha=expected_sha, started_epoch=started_epoch,
        )
        if ok:
            errors += validate_semantics(
                kind, report, client_log, players=players, run_id=run_id,
                expected_sha=expected_sha,
            )
        checks[kind] = not errors
        all_errors[kind] = errors
    return checks, all_errors
