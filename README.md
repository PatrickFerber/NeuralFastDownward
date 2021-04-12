# Neural Fast Downward
Neural Fast Downward is intended to help with generating training data for
classical planning domains, as well as, using machine learning techniques with
Fast Downward (especially, Tensorflow and PyTorch). The code was quickly 
refactored and updated to the current (March 2021) Fast Downward main branch.
Thus, it was not as well tested as it should be. If you find any bugs, please
contact me (patrick.ferber@unibas.ch) or create a pull request.

Neural Fast Downward is a fork from Fast Downward. For more information related to
Fast Downward, read the bottom part of this README.md.


## Features
### Sampling
Neural Fast Downward implement a plugin type that takes a task as input and
creates new states from its state space. Currently the following two techniques
are implemented:
- Random walks with progression from the initial state
- Random walk with regression from the goal state

Furthermore, a sampling engine is implemented that takes the above mentioned
plugins evaluates the sampled states (produces for every state a vector of
strings) and stores them to disk (in the future hopefully to named pipes). How 
the states are evaluated depends on the concrete implementation of the 
sampling engine. Examples are:
- writing the new states as SAS tasks to disk
- solving the states using a given search algorithm and storing the states along
  the plan
- store an estimate for a state by evaluating its n-step successors with a 
  given heuristic function and back propagate the minimum estimate (similar to a
  Bellman update).
  
If you are only interested in the sampling code, just work with the branch 
`sampling`. The branch `main` contains the sampling feature, as well as, the
features below.
  
### Policies
Neural Fast Downward has some simple support for policies in classical planning.
It does **not** implement a good policy, but it provides a Policy class which can
be extended, two simple policies with internally rely on a given heuristic and
a simple search engine which follows the choices of a policy.

### Neural Networks
Neural Fast Downward supports Tensorflow and PyTorch models. It implements an 
abstract neural network base class and implements a network subclass for
Tensorflow and PyTorch. If you want to support another ML library take 
inspiration by those classes.

Furthermore, a network heuristic and network policy are provided. Both are simple
wrappers that take an abstract network and take the network outputs as heuristic
resp. policy values.

**Tensorflow.**
The setup of Tensorflow has changed overtime. The code is still there, but I have
not tried to get it running with the current version of Tensorflow. I am glad for
PR that adapt the code for the current Tensorflow version (if any 
changes are necessary) and for instructions I can upload for anyone who wants to
compile their code with Tensorflow.
After setting up Tensorflow, you have to uncomment the Tensorflow Plugin in
`src/search/DownwardFiles.cmake`.

**PyTorch.**
Setting up PyTorch is straight forward. Download `torchlib` and extract it to
any path `P`. Then set an environment variable `PATH_TORCH` that points to `P`.
Afterwards, you have to uncomment the Torch Plugin in
`src/search/DownwardFiles.cmake`.



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
