# HW1 Performance Report (WSL)

## Implementation Summary
- Basic: odd-even sort with neighbor boundary swaps only.
- Advanced: local sort + neighbor split/merge in odd/even phases.

## Experimental Setup
- Machine: [fill in CPU model, cores, RAM]
- OS: WSL [distro + version]
- MPI: OpenMPI 4.1.6
- Compiler: g++/mpicxx 13.3.0

## Methodology
- Input size: n = 2^18 (262144 floats)
- Processes: P = 1, 2, 4, 8
- Trials: 3 runs each
- Timing: `/usr/bin/time -f %e` on the full program
- Input data: fixed random seed (42) for reproducibility

## Results
Averages are from `HW1/perf_results.csv`.

| Impl | P | Avg Time (s) | Notes |
|------|---|--------------|-------|
| basic | 1 | 78.202500 | |
| basic | 2 | 38.453333 | |
| basic | 4 | 26.283333 | |
| basic | 8 | 25.233333 | |
| advanced | 1 | 0.312500 | |
| advanced | 2 | 0.316667 | |
| advanced | 4 | 0.320000 | |
| advanced | 8 | 0.346667 | |

## Analysis
- Basic shows significant runtime reduction as P increases; the algorithm is O(n^2) so it remains slow.
- Advanced is orders of magnitude faster because it uses local sort + split/merge instead of pure odd-even swaps.
- For advanced, higher P slightly increases time at n=2^18 due to communication overhead and process startup costs.

## Conclusion
- Advanced greatly outperforms Basic at n=2^18.
- Scaling benefits are limited for advanced at this n on WSL; larger n or native Linux may show clearer scaling.
