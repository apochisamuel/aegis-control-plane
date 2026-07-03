# Architecture

Aegis treats an input as a capture header followed by length-delimited frames.
The capture reader verifies basic framing, decodes records, and forwards domain
records to an engine transaction. The engine stages related identity, route,
policy, charging, session, and extension updates, then commits coherent state
for dispatch, persistence, recovery, and diagnostics.

The route path creates immutable route records, rebuilds lookup projections, and
compiles dispatch plans for active sessions. The policy path validates references,
calculates quota and charging effects, and coordinates activation with dispatch
planning. The persistence path manages checkpoint pages, journal records,
snapshot restoration, and replay windows in dependency order. The capture path
tracks fragment groups and publishes trace windows for completed streams.

Diagnostics combine dispatch labels, extension projections, charge statements,
replay observations, and trace summaries without opening network connections.
All modules are deterministic: time is represented by record counters and
explicit values carried in frames rather than system-clock calls.
