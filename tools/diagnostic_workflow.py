#!/usr/bin/env python3
"""Versioned, deterministic acceptance-report and proposal helpers."""
from __future__ import annotations

import hashlib
import json
import pathlib
from typing import Any, Callable

ROOT = pathlib.Path(__file__).resolve().parents[1]
REPORT_SCHEMA_VERSION = 1
STATUSES = ("PASS", "FAIL", "NOT RUN", "BLOCKED", "UNKNOWN")
STAGES = (
    "brain-inspection", "power-rails", "usb-identity", "external-power",
    "module-1x8", "module-2x5", "navigation", "encoders", "pads",
    "complete-harness", "addressable-illumination", "encoder-illumination",
    "calibration", "combined-stress", "final-report",
)
MEASUREMENTS = (
    "supply_configuration", "supply_current_limit_ma", "input_12v_v",
    "rail_5v_v", "rail_3v3_v", "idle_current_ma",
    "representative_illumination_current_ma", "limited_white_current_ma",
    "farthest_module_voltage_v", "regulator_temperature_c",
    "ambient_temperature_c", "sk6812_timing_ns", "tlc_clock_hz",
    "tlc_latch_ns", "input_scan_jitter_us", "measurement_instrument",
    "notes",
)


def _load(path: pathlib.Path) -> dict[str, Any]:
    return json.loads(path.read_text())


def fingerprint(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def new_report(identity: dict[str, Any] | None = None) -> dict[str, Any]:
    """Return a report where absence of evidence is explicitly NOT RUN."""
    return {
        "schema_version": REPORT_SCHEMA_VERSION,
        "fixture": False,
        "overall_status": "NOT RUN",
        "firmware_identity": identity or {},
        "hardware_revision": None,
        "physical_boards": [],
        "harness": {"status": "NOT RUN", "configuration": None},
        "calibration": {"status": "NOT RUN", "configuration": None},
        "stages": [
            {"id": stage, "status": "NOT RUN", "expected": [],
             "observed": [], "automated": [], "operator_confirmations": [],
             "duration_ms": None, "notes": []}
            for stage in STAGES
        ],
        "measurements": {
            key: {"status": "NOT RUN", "value": None, "unit": None,
                  "instrument": None, "operator_entered": True}
            for key in MEASUREMENTS
        },
        "error_counters": {},
        "proposals": {"harness": None, "encoders": None,
                      "calibration": None},
        "outstanding_issues": [],
    }


def validate_report(report: dict[str, Any]) -> None:
    if report.get("schema_version") != REPORT_SCHEMA_VERSION:
        raise ValueError("unsupported report schema_version")
    if report.get("overall_status") not in STATUSES:
        raise ValueError("invalid overall_status")
    stages = report.get("stages")
    if not isinstance(stages, list) or [x.get("id") for x in stages] != list(STAGES):
        raise ValueError("report stages are incomplete or out of order")
    for stage in stages:
        if stage.get("status") not in STATUSES:
            raise ValueError(f"invalid status for {stage.get('id')}")
    for field in MEASUREMENTS:
        entry = report.get("measurements", {}).get(field)
        if not isinstance(entry, dict) or entry.get("status") not in STATUSES:
            raise ValueError(f"invalid measurement {field}")
        if entry["status"] == "PASS" and entry.get("value") is None:
            raise ValueError(f"measurement {field} cannot PASS without a value")


def stage(report: dict[str, Any], stage_id: str) -> dict[str, Any]:
    try:
        return next(x for x in report["stages"] if x["id"] == stage_id)
    except (KeyError, StopIteration) as exc:
        raise ValueError(f"unknown stage {stage_id}") from exc


def set_stage(report: dict[str, Any], stage_id: str, status: str,
              *, note: str | None = None) -> None:
    if status not in STATUSES:
        raise ValueError("invalid status")
    item = stage(report, stage_id)
    item["status"] = status
    if note:
        item["notes"].append(note)
    completed = [x["status"] for x in report["stages"]]
    report["overall_status"] = (
        "FAIL" if "FAIL" in completed else
        "BLOCKED" if "BLOCKED" in completed else
        "PASS" if all(x == "PASS" for x in completed) else "NOT RUN"
    )


def record_measurement(report: dict[str, Any], field: str, value: Any,
                       unit: str | None, instrument: str | None) -> None:
    if field not in MEASUREMENTS:
        raise ValueError("unknown measurement field")
    report["measurements"][field] = {
        "status": "PASS", "value": value, "unit": unit,
        "instrument": instrument, "operator_entered": True,
    }


def save_report(report: dict[str, Any], path: pathlib.Path) -> None:
    validate_report(report)
    path.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")


def load_report(path: pathlib.Path) -> dict[str, Any]:
    report = _load(path)
    validate_report(report)
    return report


def markdown(report: dict[str, Any]) -> str:
    validate_report(report)
    lines = ["# NEUROGENIC manufactured-board acceptance", "",
             f"Overall status: **{report['overall_status']}**", "",
             "## Identity", ""]
    ident = report["firmware_identity"]
    lines.extend([f"- Firmware: {ident.get('firmware', 'UNKNOWN')}",
                  f"- Source: {ident.get('source', 'UNKNOWN')}"
                  f" (dirty={ident.get('dirty', 'UNKNOWN')})",
                  f"- Hardware: {ident.get('hardware', 'UNKNOWN')}", "",
                  "## Stages", ""])
    lines.extend(f"- {x['id']}: {x['status']}" for x in report["stages"])
    lines.extend(["", "## Measurements", ""])
    for key, value in report["measurements"].items():
        shown = "NOT RUN" if value["value"] is None else str(value["value"])
        unit = f" {value['unit']}" if value.get("unit") else ""
        lines.append(f"- {key}: {shown}{unit} ({value['status']})")
    lines.extend(["", "## Outstanding issues", ""])
    lines.extend([f"- {x}" for x in report["outstanding_issues"]] or ["- None recorded."])
    return "\n".join(lines) + "\n"


def parse_ngd1(line: str) -> dict[str, str]:
    if len(line) > 1024 or not line.startswith("NGD1 "):
        raise ValueError("malformed diagnostic response")
    result: dict[str, str] = {}
    for token in line.rstrip().split()[1:]:
        if "=" not in token:
            raise ValueError("malformed diagnostic field")
        key, value = token.split("=", 1)
        if not key or key in result:
            raise ValueError("duplicate or empty diagnostic field")
        result[key] = value
    if "type" not in result:
        raise ValueError("diagnostic response lacks type")
    return result


def validate_identity(identity: dict[str, str]) -> None:
    if identity.get("type") != "identity":
        raise ValueError("not an identity response")
    if identity.get("diag") != "1":
        raise ValueError("unsupported diagnostic interface version")
    if identity.get("dirty") not in ("0", "1"):
        raise ValueError("invalid dirty-tree identity")
    if identity.get("dirty") == "0" and len(identity.get("source", "")) != 40:
        raise ValueError("clean build lacks exact source revision")


def require_capability(capabilities: set[str], required: str) -> None:
    if required not in capabilities:
        raise ValueError(f"firmware lacks required capability: {required}")


def module_profiles() -> dict[str, list[str]]:
    return {
        "brain-only": [], "one-1x8": ["buttons-1x8"],
        "one-2x5": ["buttons-2x5"], "navigation": ["nav"],
        "encoder": ["encoders"], "pad": ["pads"],
        "complete-reference": ["buttons-1x8", "buttons-1x8",
                               "buttons-2x5", "buttons-2x5", "nav", "pads",
                               "encoders"],
    }


def propose_harness(profile: str) -> dict[str, Any]:
    manifest = _load(ROOT / "hardware/manifest.json")
    requested = module_profiles().get(profile)
    if requested is None:
        raise ValueError("unknown diagnostic harness profile")
    chain = {name: len(board["registers"]) * 8
             for name, board in manifest["board_types"].items()
             if "registers" in board}
    modules = [x for x in requested if x != "encoders"]
    ports = []
    for index in range(8):
        module = modules[index] if index < len(modules) else "unused"
        ports.append({"port": f"MOD{index + 1}", "module_type": module,
                      "logical_instance": f"proposed-{module}-{index + 1}",
                      "chain_bits": chain.get(module, 0),
                      "enabled": module != "unused", "analog": None})
    encoder = "encoders" if "encoders" in requested else "unused"
    ports.append({"port": "ENC", "module_type": encoder,
                  "logical_instance": "proposed-encoders",
                  "chain_bits": chain.get(encoder, 0),
                  "enabled": encoder != "unused", "analog": None})
    return {"schema_version": 1, "status": "PROPOSED_OPERATOR_REVIEW_REQUIRED",
            "source_manifest_sha256": fingerprint(ROOT / "hardware/manifest.json"),
            "profile": profile, "ports": ports}


def propose_encoder(name: str, observed_cw: int, observed_ccw: int,
                    known_steps: int) -> dict[str, Any]:
    if not name or observed_cw == 0 or observed_ccw == 0 or known_steps <= 0:
        raise ValueError("both directions and a positive known extent are required")
    counts = max(1, round((abs(observed_cw) + abs(observed_ccw)) /
                          (2 * known_steps)))
    return {"encoder": name, "invert": observed_cw < 0,
            "counts_per_increment": counts,
            "evidence": {"cw_raw": observed_cw, "ccw_raw": observed_ccw,
                         "known_steps": known_steps},
            "status": "PROPOSED_OPERATOR_REVIEW_REQUIRED"}


def propose_calibration(configuration: dict[str, Any]) -> dict[str, Any]:
    if configuration.get("schema_version") != 1:
        raise ValueError("unsupported calibration schema")
    gains = configuration.get("driver_gain_q8", {})
    for driver in ("sk6812", "tlc5947"):
        values = gains.get(driver)
        if not isinstance(values, list) or len(values) != 3 or any(
                not isinstance(x, int) or not 0 <= x <= 512 for x in values):
            raise ValueError("invalid calibration gains")
    result = json.loads(json.dumps(configuration))
    result["status"] = "PROPOSED_OPERATOR_REVIEW_REQUIRED"
    result["source_manifest_sha256"] = fingerprint(ROOT / "hardware/manifest.json")
    result["source_calibration_sha256"] = fingerprint(
        ROOT / "hardware/illumination_calibration.json")
    return result


def run_prerequisite(prompt: Callable[[str], str], text: str) -> bool:
    """Only an explicit operator yes advances a safety prerequisite."""
    return prompt(text + " [yes/no]: ").strip().lower() == "yes"


def guided_button_result(expected: str, events: list[dict[str, Any]]) -> dict[str, Any]:
    """Classify already-bounded decoded events without pretending operator evidence."""
    matching = [e for e in events if e.get("control") == expected]
    unexpected = [e for e in events if e.get("control") != expected]
    presses = sum(e.get("event") == "PRESS" for e in matching)
    releases = sum(e.get("event") == "RELEASE" for e in matching)
    reasons = []
    if presses != 1: reasons.append("missing-or-duplicate-press")
    if releases != 1: reasons.append("missing-or-duplicate-release")
    if unexpected: reasons.append("unexpected-simultaneous-transition")
    return {"status": "PASS" if not reasons else "FAIL", "expected": expected,
            "presses": presses, "releases": releases,
            "unexpected": unexpected, "reasons": reasons,
            "operator_confirmed_identity": False}


def query_endpoint(endpoint: Any, command: str) -> list[dict[str, str]]:
    """Exercise strict response parsing against a serial-like test endpoint."""
    if len(command) > 63:
        raise ValueError("command too long")
    lines = endpoint.exchange(command, timeout_s=1.0, max_bytes=65536)
    if not lines:
        raise TimeoutError("diagnostic response timeout")
    return [parse_ngd1(line) for line in lines]


def safe_session(endpoint: Any, commands: list[str]) -> list[list[dict[str, str]]]:
    replies = []
    try:
        for command in commands:
            replies.append(query_endpoint(endpoint, command))
        return replies
    finally:
        try:
            endpoint.exchange("led off", timeout_s=0.2, max_bytes=1024)
        except (OSError, TimeoutError):
            pass
