# Neural Fast Downward
Neural Fast Downward generates training data for
classical planning domains and provides support for Protobuf (Tensorflow 1.x)
and PyTorch models. The refactored code is not as well tested as it should 
be. Please report bugs to **patrick.ferber@unibas.ch** or create a pull request.

For more information related to Fast Downward, consult the bottom part of 
this README.md.

If you use Neural Fast Downward in your research, please cite

```
@InProceedings{ferber-et-al-ecai2020,
  author =       "Patrick Ferber and Malte Helmert and J{\"o}rg Hoffmann",
  title =        "Neural Network Heuristics for Classical Planning: A Study of
                  Hyperparameter Space",
  pages =        "2346--2353",
  booktitle =    "Proceedings of the 24th {European} Conference on
                  {Artificial} {Intelligence} ({ECAI} 2020)",
  year =         "2020"
}
```


## Features
### Sampling
Neural Fast Downward generates data from a given task,
therefore, it uses **SamplingTechniques** which take a given task and modify
it and **SamplingEngines** which perform some action with the modified task.

**Current Sampling Techniques**:
- New initial state via random walks with progression from the original initial
  state
- New initial state via random walk with regression from the goal condition 


**Current Sampling Engines:**
- writing the new states as (partial) SAS tasks to disk
- use a given search algorithm to find plans for the new states and 
  store them
- estimate the heuristic value of a state by value update using the n-step 
  successors (like Bellman update).
  
If you are only interested in the sampling code, just work with the branch 
`sampling`. The branch `main` contains the sampling feature, as well as, the
features below.

**Example:**

Generate two state via regression from the goal with random walk lengths
 between 5 and 10. Use `A*(LMcut)` to find a solution and store all states
  along the plan, as well as the used operators. 

```./fast-downward.py --build BUILD ../benchmarks/gripper/prob01.pddl --search 
"sampling_search_simple(astar(lmcut(transform=sampling_transform()),transform=sampling_transform()), techniques=[gbackward_none(2, distribution=uniform_int_dist(5, 10))])"
```

*ATTENTION: By default, the components of Fast Downward (e.g. search engines 
and heuristics) use the original task. Thus, you have to provide them the
argument `transform=sampling_transform()`.*
  

  
[Click here for more information and examples](SAMPLING.md)
  
### Policies
Neural Fast Downward has some simple support for policies in classical planning.
It does **not** implement a good policy, but it provides a Policy class which can
be extended. Currently, two simple policies which internally rely on a given heuristic and
a simple search engine which follows the choices of a policy are implemented.

### Neural Networks
Neural Fast Downward supports Protobuf (Tensorflow 1.x) and PyTorch models. It 
implements an 
abstract neural network base class and implements subclass for
Tensorflow and PyTorch. Wrappers which use a NN to calculate a heuristic or
policy are implemented.

**Tensorflow.**
The Tensorflow setup has changed multiple times.  The code is still there, 
but is not tested with the current version of Tensorflow (most likely will 
not run). I am glad for
PR that adapt the code for the current Tensorflow version or for instructions
After setting up Tensorflow, you have to uncomment the Tensorflow Plugin in
`src/search/DownwardFiles.cmake`.

**PyTorch.**
Setting up PyTorch is straight forward. Download `torchlib` and extract it to
any path `P`. Then set an environment variable `PATH_TORCH` that points to `P`.
Afterwards, you have to uncomment the Torch Plugin in
`src/search/DownwardFiles.cmake`.

[Click here for more information and examples](NEURALNETWORKS.md)

# Fast Downward

Fast Downward is a domain-independent classical planning system.

Copyright 2003-2020 Fast Downward contributors (see below).

For further information:
- Fast Downward website: <http://www.fast-downward.org>
- Report a bug or file an issue: <http://issues.fast-downward.org>
- Fast Downward mailing list: <https://groups.google.com/forum/#!forum/fast-downward>
- Fast Downward main repository: <https://github.com/aibasel/downward>


## Tested software versions

This version of Fast Downward has been tested with the following software versions:

| OS           | Python | C++ compiler                                                     | CMake |
| ------------ | ------ | ---------------------------------------------------------------- | ----- |
| Ubuntu 20.04 | 3.8    | GCC 9, GCC 10, Clang 10, Clang 11                                | 3.16  |
| Ubuntu 18.04 | 3.6    | GCC 7, Clang 6                                                   | 3.10  |
| macOS 10.15  | 3.6    | AppleClang 12                                                    | 3.19  |
| Windows 10   | 3.6    | Visual Studio Enterprise 2017 (MSVC 19.16) and 2019 (MSVC 19.28) | 3.19  |

We test LP support with CPLEX 12.9, SoPlex 3.1.1 and Osi 0.107.9.
On Ubuntu, we test both CPLEX and SoPlex. On Windows, we currently 
only test CPLEX, and on macOS, we do not test LP solvers (yet).


## Contributors

The following list includes all people that actively contributed to
Fast Downward, i.e. all people that appear in some commits in Fast
Downward's history (see below for a history on how Fast Downward
emerged) or people that influenced the development of such commits.
Currently, this list is sorted by the last year the person has been
active, and in case of ties, by the earliest year the person started
contributing, and finally by last name.

- 2003-2020 Malte Helmert
- 2008-2016, 2018-2020 Gabriele Roeger
- 2010-2020 Jendrik Seipp
- 2010-2011, 2013-2020 Silvan Sievers
- 2012-2020 Florian Pommerening
- 2013, 2015-2020 Salome Eriksson
- 2016-2020 Cedric Geissmann
- 2017-2020 Guillem Francès
- 2018-2020 Augusto B. Corrêa
- 2018-2020 Patrick Ferber
- 2015-2019 Manuel Heusner
- 2017 Daniel Killenberger
- 2016 Yusra Alkhazraji
- 2016 Martin Wehrle
- 2014-2015 Patrick von Reth
- 2015 Thomas Keller
- 2009-2014 Erez Karpas
- 2014 Robert P. Goldman
- 2010-2012 Andrew Coles
- 2010, 2012 Patrik Haslum
- 2003-2011 Silvia Richter
- 2009-2011 Emil Keyder
- 2010-2011 Moritz Gronbach
- 2010-2011 Manuela Ortlieb
- 2011 Vidal Alcázar Saiz
- 2011 Michael Katz
- 2011 Raz Nissim
- 2010 Moritz Goebelbecker
- 2007-2009 Matthias Westphal
- 2009 Christian Muise


## History

The current version of Fast Downward is the merger of three different
projects:

- the original version of Fast Downward developed by Malte Helmert
  and Silvia Richter
- LAMA, developed by Silvia Richter and Matthias Westphal based on
  the original Fast Downward
- FD-Tech, a modified version of Fast Downward developed by Erez
  Karpas and Michael Katz based on the original code

In addition to these three main sources, the codebase incorporates
code and features from numerous branches of the Fast Downward codebase
developed for various research papers. The main contributors to these
branches are Malte Helmert, Gabi Röger and Silvia Richter.


## License

The following directory is not part of Fast Downward as covered by
this license:

- ./src/search/ext

For the rest, the following license applies:

```
Fast Downward is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

Fast Downward is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <https://www.gnu.org/licenses/>.
```
