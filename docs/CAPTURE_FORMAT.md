# Capture Format

A capture begins with the ASCII magic `AGCP`, a one-byte format version, and a
reserved byte. Frames follow as `[kind:u8][flags:u8][size:u16-le][payload]`.
Unknown frame kinds are preserved as audit observations. Frame payloads use
little-endian integer fields, bounded UTF-8 text, and nested length-delimited
extension blocks.

The library accepts captures only when all frame boundaries are contained within
the input. Domain-level validation is performed after decoding, so an otherwise
well-framed capture can still produce a report containing rejected operations.
