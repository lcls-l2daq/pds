import sys

prev_ts = None

for l in sys.stdin:
   # if "Enc Timestamp" not in l:
   #    continue
   if "ts=" not in l:
      continue

   # ts_start = l.index(": ") + 1
   # ts_end   = l.index(".")
   ts_start = l.index("ts=") + 3
   ts_end   = l.index(", ch0")
   ts = long( l[ts_start:ts_end] )

   if prev_ts is None:
      prev_ts = ts
      continue

   print "Diff:", (ts - prev_ts) * .030110114, "us."
   # print "Diff:", (ts - prev_ts), "counts."
   prev_ts = ts

