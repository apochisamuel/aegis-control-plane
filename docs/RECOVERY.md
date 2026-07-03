# Recovery

Snapshots store value copies of durable subscriber, session, policy, route, and
quota state. Journals preserve ordered transitions that can be replayed after a
snapshot is loaded. Recovery processes identity and routing state before policy
and session state so references can be checked as they are restored.
