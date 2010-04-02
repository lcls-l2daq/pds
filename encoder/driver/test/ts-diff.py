import sys

prev_ts = None

for l in sys.stdin:
   if "Entry #" not in l:
      continue

   ts_start = l.index("=") + 1
   ts_end   = l.index(",")
   ts = long( l[ts_start:ts_end] )

   if prev_ts is None:
      prev_ts = ts
      continue

   print "Diff:", (ts - prev_ts) * .03, "us."
   prev_ts = ts

