# Aegis Control Plane

Aegis Control Plane is an offline C++20 library for decoding, validating,
replaying, inspecting, and recovering structured telecommunications
control-plane captures. It models subscriber registration, session and bearer
state, tunnel bindings, route selection, staged policy, quota usage, replay
windows, snapshots, transaction journals, extension fields, and diagnostic
reports.

## Build

```bash
cmake -S . -B build -DAEGIS_BUILD_TESTS=ON -DAEGIS_BUILD_TOOLS=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

No package downloads, credentials, service endpoints, or network access are
required. The capture format is documented in `docs/CAPTURE_FORMAT.md`.

## Components

- Binary codec, framing, capture, and reassembly layers.
- Subscriber, session, bearer, tunnel, policy, quota, routing, and handover
  state machines.
- Snapshot, journal, replay, recovery, audit, metrics, and diagnostics tools.
- Four independent libFuzzer entry points with non-crashing seed corpora.

This source tree intentionally omits crash reproducers, patches, and internal
validation notes from the release archive.
