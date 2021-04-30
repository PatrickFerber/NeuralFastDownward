# Sampling

## Sampling Techniques

- Code: `src/search/sampling_techniques`
- Base class: `SamplingTechnique`
- Purpose: Generate from a given task a new task (e.g. by changing the initial
  state)
  
- Examples:
    - Modify the initial state by progression random walk:
    ```iforward_none(NB_OF_TASKS_TO_GENERATE, distribution=uniform_int_dist(MIN_WALK_LENGTH, MAX_WALK_LENGTH))```

    - Generate a new initial state by regression random walk from the goal and
    afterwards completing the partial assignment to a state:
    ```backward_none(NB_OF_TASKS_TO_GENERATE, distribution=uniform_int_dist(MIN_WALK_LENGTH, MAX_WALK_LENGTH))```
   - Please consult the code to see the full list of implemented techniques and
    all their parameters
     
## Sampling Engines

- Code: `src/search/sampling_engines`
- Purpose: Use the sampling techniques to produce new tasks and extract 
  training data from those tasks.
  
**Examples:**
- Generate two new states via regression walks from the goal with walk lengths
  between 5 and 10. The states are solved using `A*(LMcut)`. By default the
  samples are written to `sas_plan`.
```
./fast-downward.py --build debug ../benchmarks/gripper/prob01.pddl --search 
"sampling_search_simple(astar(lmcut(transform=sampling_transform()),
transform=sampling_transform()), techniques=[gbackward_none(2, 
distribution=uniform_int_dist(5, 10))])"
```

For all SamplingEngines and all parameters, take a look at the code. Below 
are the most important ones.
### SamplingEngine
- `src/search/sampling_engines/sampling_engine.{h, cc}`
- manages the sampling techniques
    - queries them for new tasks, shuffles their order if desired, and 
      stops once all SamplingTechniques are exhausted.
- passes the new task to the **abstract** `sample(task)` method of its subclass 
  and 
  expects a list of strings (aka samples) back.
- manages the samples (when and where to store them)

### SamplingSearchBase : SamplingEngine
- `src/search/sampling_engines/sampling_search_base.{h, cc}`
- receives as additional parameter a search engine configuration 
  (predefinitions are possible). For every new task, it creates the search 
  engine object anew and executes the search. An **abstract** method 
  extracts the samples from the search engine object.
  
### SimpleSamplingSearch : SamplingSearchBase
- `src/search/sampling_engines/sampling_search_simple.{h, cc}`
- This engine extracts from the plan of an successful search
  - the states along the plan
  - the remaining path cost for the state
  - the operators used
    
    