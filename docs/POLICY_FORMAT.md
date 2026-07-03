# Policy Format

Policy records contain a policy identifier, revision, route selector, priority,
quota ceiling, and a compact list of charging clauses. A policy transaction is
staged until its route, subscriber generation, and charging references are
available. Activation produces a policy snapshot used by dispatch planning and
recovery.
