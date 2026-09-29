# DES from Scratch — Sender/Receiver Demo

A from-scratch (no crypto library) implementation of standard DES
(FIPS 46-3), used to demonstrate symmetric encrypted communication
between two machines over TCP, in **CBC mode**, with a full
round-by-round log of every transformation.

The two programs now hold a **persistent, two-way chat session**: after
connecting once, they take turns sending and receiving encrypted
messages back and forth until either side types `exit` or `quit`.

## Files

| File                  | Purpose                                                          |
|------------------------|-------------------------------------------------------------------|
| `des.hpp`              | Core DES algorithm: tables, key schedule, block encrypt/decrypt   |
| `crypto_utils.hpp`     | PKCS#7 padding, CBC XOR chaining, random IV generation            |
| `network.hpp`          | Plain POSIX TCP socket helpers (Linux/macOS)                      |
| `message_protocol.hpp` | Message framing + encrypt-and-send / receive-and-decrypt + trace |
| `sender.cpp`           | Program run on the machine that opens the conversation            |
| `receiver.cpp`         | Program run on the machine that waits for the conversation         |

## Building

On **both** machines:

```bash
make
```

Each program only needs its own `.cpp` plus the three `.hpp` headers in
the same folder — clone this repository or copy manually the whole folder to both machines.

## Running (two real machines on a LAN)

1. **On the receiver machine**, find its LAN IP (e.g. `ip addr` / `ifconfig`
   on Linux, `ipconfig` on Windows), then start it first:
   ```bash
   ./receiver
   ```
   It will ask for:
   - a port to listen on (e.g. `55555`) — make sure it's not blocked by a firewall
   - the shared 8-character DES key

2. **On the sender machine**, run:
   ```bash
   ./sender
   ```
   It will ask for:
   - the receiver's IP address (from step 1)
   - the receiver's port (must match)
   - the **same** 8-character shared key

3. Once connected, the two programs take **strict turns**:
   - The sender types a message → it's encrypted (full round trace
     printed) and sent → the sender then waits.
   - The receiver's window shows the decrypted message (full trace
     printed) → the receiver types a reply → it's encrypted and sent
     back → the receiver then waits.
   - Back to the sender, and so on, indefinitely.

4. **To end the session**, type `exit` or `quit` at either side's
   prompt. That side sends a small "end of conversation" signal to the
   other, and both programs shut down gracefully and print `DONE`.

> The key must be **exactly 8 characters** (DES uses a 64-bit key) and
> must be identical on both sides — this models the "both sender and
> receiver already share the key" symmetric-encryption assumption. The
> key schedule (16 round keys) is derived once at startup and reused
> for every message in the session; only the IV is randomized per
> message.

## How it fits together

- **DES core (`des.hpp`)**: standard Initial/Final Permutation, 16
  Feistel rounds (Expansion → XOR round key → 8 S-boxes → P
  permutation), and the standard PC-1/PC-2 key schedule with the
  official per-round left-shift schedule. Decryption reuses the exact
  same function, just walking the 16 round keys in reverse order
  (this is what makes DES's Feistel structure self-inverting).
- **CBC mode (`crypto_utils.hpp`)**: since DES only ever encrypts a
  single 8-byte block, longer messages are PKCS#7-padded to a multiple
  of 8 bytes, split into blocks, and chained: each plaintext block is
  XORed with the *previous ciphertext block* (or the IV, for the first
  block) before being run through DES. Decryption reverses this: DES
  output is XORed with the previous ciphertext block to recover
  plaintext.
- **Wire protocol (`message_protocol.hpp`)**: a single TCP connection
  stays open for the whole session. Every message, in either
  direction, is framed as `[4-byte block count][8-byte IV][ciphertext
  bytes]`. The IV itself doesn't need to be secret in CBC, only
  unpredictable, so it's sent in the clear alongside the ciphertext —
  this is standard practice. A block count of `0` is a reserved
  sentinel meaning "I'm ending the conversation" (a real message
  always pads to at least one block, so this never collides with
  real data); it's what `exit`/`quit` sends instead of a message.
- **Turn order**: the sender always sends first after connecting; the
  receiver always waits to receive first. This is why no background
  threads are needed — each side is only ever blocked on one thing
  (typing or receiving) at a time.

## What the log shows you

For every block, you'll see:
- the plaintext block, XORed with the previous cipher block (CBC step)
- after Initial Permutation: `L0`/`R0`
- each of the 16 rounds: the expansion `E(R)`, XOR with the round key,
  each S-box's 4-bit output concatenated, the `P()` permutation output,
  and the resulting new `L`/`R`
- the final swap + Final Permutation producing the ciphertext block

The receiver's log mirrors this exactly in reverse (same rounds, keys
used K16→K1), letting you compare side-by-side how encryption and
decryption are structurally the same operation.

## Notes

- Bits are represented as `std::vector<int>` (one int per bit) rather
  than packed integers — much easier to read/step through/debug when
  learning the algorithm, at the cost of raw performance.
- This is a **teaching implementation** of single-DES (56-bit
  effective key), not Triple-DES, and CBC mode has no message
  authentication (MAC) — don't use this for real security purposes.
- Networking uses plain POSIX sockets with **no TLS/authentication of
  its own** — the DES layer is the entire "security," matching the
  scope of the assignment (demonstrating symmetric encryption itself).
- Key mismatches between sender and receiver cause decryption to produce
  garbage, which is reported as a decryption failure (rather than a connection
  error). Note that in roughly 1 out of 256 cases garbage can accidentally look
  like valid PKCS#7 padding, so a mismatch may occasionally go undetected until
  the next message.
- The chat is **strict-turn** (send → wait for reply → send → ...),
  not free-form — a side can't send a second message before the other
  has replied. This keeps the code simple (no threads/async I/O). If
  you'd rather have a live, type-anytime chat instead, that needs a
  background thread per side to listen while you type; ask if you
  want that version.
