# R14.43 LAB — Mainline Workbench history reconciliation

This is a **LAB-only** ancestry reconciliation. The R14.39/R14.41/R14.42
369-case LAB tree remains authoritative for this branch. No change to
SquachWatch firmware, RF modulation, emitter hardware, or production main.

At the divergence point, `main` gained three website-only commits:

- `88f3066f`: public 369-case Workbench, Freenove receiver selector,
  fork/upstream labels and source-pinned catalog.
- `98b161c3`: explicitly authorized unverified transmitter-only mode
  when the receiver lacks structured serial LOG.
- `7aaab64d`: stop batch on unconfirmed TRUTH BEGIN/END, preserving
  checkpoint progress; provide Remaining (successful END excluded) scope.

Each feature was reviewed against the integrated LAB version before
recording the common ancestry. The LAB files already implement the
same workflows with the subsequent R14.41/42 fixes for provenance,
checkpoint integrity and receiver scan evidence. The merged contents
deliberately retain the newer LAB implementations of the nine
website files changed on main, rather than replacing them with their
older public versions.

This history reconciliation does **not** assert hardware validation.
CI builds are necessary but cannot prove successful physical radio
transmissions, continuous SquachWatch scanning, or faithful LCD pixels.
Do not merge the LAB firmware into `main` or mark PR #113 as
hardware-approved based on this ancestry repair.
