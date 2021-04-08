#!/bin/bash
../../fast-downward.py --build debug ~/repositories/benchmarks/gripper/prob01.pddl --search "astar(nh(ttest(path=traced.pt)))"

