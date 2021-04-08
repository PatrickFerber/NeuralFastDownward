#!/bin/bash

../../fast-downward.py --build debug64dynamic test.pddl --search "eager_greedy([nh(network=sgnet(type=regression,path=model.pb, state_layer=input_1, goal_layer=input_2,output_layers=[dense_6/Relu]))], cost_type=ONE)"
# atoms & defaults does not need to be set, because their automatic detection is sufficient
