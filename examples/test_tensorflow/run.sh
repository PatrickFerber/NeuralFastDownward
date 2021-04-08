#!/bin/bash

MODEL="best_model.pb"
../../fast-downward.py --build debug p02.pddl \
  --search "eager_greedy([nh(network=snet(type=classification,path=${MODEL}, state_layer=dense_1_input, output_layers=[output_layer/Softmax]))], cost_type=ONE)"
