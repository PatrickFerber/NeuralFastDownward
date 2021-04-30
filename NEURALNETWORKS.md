# Neural Networks
- Code: `src/search/neural_networks`
- Base class: `AbstractNetwork`

## The Basics
`AbstractNetwork` defines the interface for all networks. Every network
has to implement a function to evaluate a state and every network has to declare
which outputs it provides (e.g. heuristic value and preferred operators), 
as well as, provide access to the outputs.

## Adding a new output type (e.g. confidence)
If you want your network to output some new information, e.g. the confidence
in its heuristic estimate, then you have to modify the `AbstractNetwork` class.
Add the following four methods:
- `is_FEATURE()`: This method denotes if the concrete network class produced 
  output for your feature. Its default implementation should return `false`.
- `verify_FEATURE()`: This method shall terminate the executing, if the network
  object does not support your feature.
- `RETURN_TYPE get_FEATURE()`: After calling `evaluate(State)`, the (interpreted)
  network output should be internally stored and can be accessed by this method. 
- `std::vector<RETURN_TYPE> get_FEATUREs()`: After calling `evaluate(vector<State>)`
  the interpreted output for all states in the vector can be accessed by this
  function. If `evaluate(State)` was called, then this function should return 
  a vector of size 1.

## Adding support for a new ML framework

First, I suggest that you make yourself familiar with the C++ API of your
framework. Write a simple example file that loads a model of your framework,
feeds inputs into the model and extracts the output. Then try to compile and
run it.

Once this was successful, add a new subclass of `AbstractNetwork`, e.g. 
`FRAMEWORKNetwork`. In its constructor and or initialize method do all the
preparation of your test script. Do not forget to free any memory in the
destructor. Implement the `evaluate` functions for your framework. That means,
extract the required input data from the given state(s) and store it in the 
appropriate format for your framework. Feed the data to the network
and then interpret the output.

The most problematic part is adapting *CMake* such that your framework is
used during compilation. Please take inspiration from `src/cmake_modules/Find*.cmake`.

For inspiration take a look at the `ProtobufNetwork` and `TorchNetwork` class.
I suggest that the input extraction and the output interpretation are done by
abstract functions. Then, you can implement many different concrete networks
for your framework.

## Adding a new network
Decide which Machine Learning Framework you want to use. Then extend the base
class for that framework, e.g. `ProtobufNetwork` for Tensorflow or `TorchNetwork`
for PyTorch. Then implement the abstract methods of that base class. Often
those:
- convert a state to the right input format for the network
- parse the output of the network

For every output the network produces you have to overwrite:
- `is_OUTPUT()`: return `true`
- `RETURN_TYPE get_OUTPUT()`: return a single output of the right type 
- `std::vector<RETURN_TYPE> get_OUTPUTs()`: return a vector of results for
   the possible vector of evaluated states.


For a simple example, take a look at the class `TestTorchNetwork`.