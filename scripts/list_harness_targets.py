#!/usr/bin/env python3

from __future__ import annotations

import sys

import harness_targets


if __name__ == "__main__":
    if len(sys.argv) == 1:
        sys.argv.append("list")
    sys.exit(harness_targets.main())
