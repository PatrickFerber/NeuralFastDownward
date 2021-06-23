import sys
if sys.version_info < (3,):
    module_not_found_error = ImportError
else:
    module_not_found_error = ModuleNotFoundError

try:
    from src.translate import pddl_parser
except module_not_found_error:
    pddl_parser = None

import json
import math
import os
import re


PATTERN_PDDL_ATOMS = re.compile("{PDDL_ATOMS[_A-Za-z0-9]*}")
PATTERN_PDDL_INITS = re.compile("{PDDL_INITS[_A-Za-z0-9]*}")
PATTERN_PROBLEM_IDX = re.compile(".*?(\d+)\.pddl")  # TODO: Improve Regex
PATTERN_MODEL_OUTPUT_LAYER = re.compile("{MODEL_OUTPUT_LAYER,[^}]+}")

MODEL_OUTPUT_LAYER_ANKER1 = b"Identity"
MODEL_OUTPUT_LAYER_ANKER2 = b"*"


def get_task_file(args):
    if len(args.translate_inputs) > 0:
        return args.translate_inputs[-1]
    else:
        assert len(args.filenames) == 1
        return args.filenames[0]


class Placeholders(object):
    PDIR = "PDIR"  # Problem file directory
    DDIR = "DDIR"  # Domain file directory
    FOLD = "FOLD"  # Determine fold to which the problem belongs (10 fold out of 200 started indexing by 1)
    PDDL_ATOMS = "PDDL_ATOMS"
    PDDL_INIT = "PDDL_INITS"
    SAS_FACTS = "SAS_FACTS"
    SAS_INIT = "SAS_INITS"
    SPLIT = "SPLIT"
    MODEL_OUTPUT_LAYER = "MODEL_OUTPUT_LAYER"

    @staticmethod
    def present(dynamic, placeholders):
        keys = set()
        for ph in placeholders:
            if ph.startswith(Placeholders.PDDL_ATOMS):
                matches = PATTERN_PDDL_ATOMS.findall(dynamic)
                for match in matches:
                    keys.add(match[1:-1])
            elif ph.startswith(Placeholders.PDDL_INIT):
                matches = PATTERN_PDDL_INITS.findall(dynamic)
                for match in matches:
                    keys.add(match[1:-1])
            elif ph == Placeholders.MODEL_OUTPUT_LAYER:
                matches = PATTERN_MODEL_OUTPUT_LAYER.findall(dynamic)
                for match in matches:
                    keys.add(match[1:-1])
            else:
                if dynamic.find("{" + ph + "}") > -1:
                    keys.add(ph)
        return keys

    @staticmethod
    def mapping(args, keys):
        atom_lists = None
        mapping = {}
        for key in keys:
            if key == Placeholders.PDIR:
                assert len(args.translate_inputs) == 2
                v = os.path.dirname(get_task_file(args))

            elif key == Placeholders.DDIR:
                assert len(args.translate_inputs) == 2
                v = os.path.dirname(args.translate_inputs[0])

            elif key == Placeholders.FOLD:
                match = PATTERN_PROBLEM_IDX.match(get_task_file(args))
                if match is None:
                    raise ValueError("Can only use {%s} with problem files of "
                                     "of the schema '%s'." %
                                     (key, PATTERN_PROBLEM_IDX.pattern))
                idx = int(match.groups()[0])
                if idx < 1 or idx > 200:
                    raise ValueError("Fold detection only supported for "
                                     "problem indices between 1 and 200 "
                                     "(inclusive).")
                v = int(math.floor((idx - 1) / 20.0))

            elif key.startswith(Placeholders.PDDL_ATOMS) or key == Placeholders.SAS_FACTS:
                if atom_lists is None:
                    atom_lists = load_atom_list(path_problem=get_task_file(args))
                if key not in atom_lists:
                    raise ValueError("Cannot find %s in the associated atom list" % key)
                v = "[" + ",".join(atom_lists[key])+"]"

            elif key.startswith(Placeholders.PDDL_INIT) or key == Placeholders.SAS_INIT:
                if atom_lists is None:
                    atom_lists = load_atom_list(path_problem=get_task_file(args))
                if key == Placeholders.SAS_INIT:
                    new_key = Placeholders.SAS_FACTS
                else:
                    new_key = key.replace(Placeholders.PDDL_INIT, Placeholders.PDDL_ATOMS, 1)
                if new_key not in atom_lists:
                    raise ValueError("Cannot find %s (for %s) in the associated atom list" % (new_key, key))
                atoms = atom_lists[new_key]

                v = "[" + ",".join(
                    load_atom_init(args.translate_inputs[0],
                                   args.translate_inputs[1], atoms)) + "]"

            elif key == Placeholders.SPLIT:
                v = None
            elif key.startswith(Placeholders.MODEL_OUTPUT_LAYER):
                file_model = key[len(Placeholders.MODEL_OUTPUT_LAYER) + 1:]
                v = extract_final_model_layer(file_model)
            else:
                assert False
            mapping[key] = v
        return mapping


def tokenizer(s):
    toks = re.compile('\s+|[()]|[^()\s]+', re.DOTALL)
    white = re.compile('\s+', re.DOTALL)
    for match in toks.finditer(s):
        s = match.group(0)
        if white.match(s[0]):
            continue
        if s[0] in '()':
            yield ("CMD", s)
        else:
            yield ('WORD', s)


def generate_parsetree(s):
    tokens = tokenizer(s)
    root = []
    current = root
    stack = [root]

    while True:
        token = next(tokens)
        if token[0] == "CMD" and token[1] == "(":
            new = []
            current.append(new)
            stack.append(new)
            current = new

        elif token[0] == "CMD" and token[1] == ")":
            stack.pop()
            current = stack[-1]

        elif token[0] == "WORD":
            current.append(token[1])

        else:
            assert False

        if current == root:
            break
    return root[0]


def load_atom_list(path_problem):
    possible_paths = [os.path.splitext(path_problem)[0] + ".atoms",
                      os.path.join(os.path.dirname(path_problem), "atoms.json")]

    for path in possible_paths:
        if os.path.exists(path):
            with open(path, "r") as f:
                return json.load(f)
    raise ValueError("Unable to find the analyse atom list for the problem and "
                     "dynamic analysis is not supported currently.")


def load_atom_init(path_domain, path_problem, atoms):
    pddl_task = pddl_parser.open(path_domain, path_problem)
    inits = set(str(x) for x in pddl_task.init)
    return ["1" if x in inits else "0" for x in atoms]


def extract_final_model_layer(file_model):
    with open(file_model, "rb") as f:
        model = f.read()
    idx1 = model.rfind(MODEL_OUTPUT_LAYER_ANKER1)
    assert idx1 != -1
    idx1 += len(MODEL_OUTPUT_LAYER_ANKER1) + 2
    idx2 = model.find(MODEL_OUTPUT_LAYER_ANKER2, idx1)

    model_output_layer = model[idx1:idx2]
    if sys.version_info >= (3,):
        model_output_layer = model_output_layer.decode("utf-8")
    return model_output_layer




def replace_options_for_dynamic(dynamic, type, args):
    global pddl_parser
    if pddl_parser is None:
        sys.path.append(os.path.join(os.path.dirname(os.path.dirname(__file__)),
                                     "builds", args.build, "bin"))
        from translate import pddl_parser

    # Check which of the placeholders valid for the argument type are present
    if type == "pb_network":
        keys = Placeholders.present(
            dynamic, [Placeholders.PDIR, Placeholders.DDIR, Placeholders.FOLD,
                      Placeholders.PDDL_ATOMS, Placeholders.PDDL_INIT,
                      Placeholders.SAS_FACTS, Placeholders.SAS_INIT,
                      Placeholders.SPLIT, Placeholders.MODEL_OUTPUT_LAYER])
    elif type in ["network", "evaluator", "heuristic", "search"]:
        keys = Placeholders.present(
            dynamic, [Placeholders.PDIR, Placeholders.DDIR, Placeholders.FOLD,
                      Placeholders.PDDL_ATOMS, Placeholders.PDDL_INIT,
                      Placeholders.SAS_FACTS, Placeholders.SAS_INIT,
                      Placeholders.MODEL_OUTPUT_LAYER])
    else:
        assert False, "Unknown kind of dynamic command: %s" % type

    # Create only once the replacement strings
    mapping = Placeholders.mapping(args, keys)

    # Modify the arguments
    if Placeholders.SPLIT in keys:
        dynamic = [x for x in dynamic.split("{%s}" % Placeholders.SPLIT)
                   if len(x) > 0]
    else:
        dynamic = [dynamic]
    for k, v in mapping.items():
        if k.startswith(Placeholders.MODEL_OUTPUT_LAYER):
            dynamic = [x.replace("{%s}" % k, "[%s]" % v) for x in dynamic]
    dynamic = [x.format(**mapping) for x in dynamic]
    return dynamic


def set_options_for_dynamic(dynamic, type, args):
    dynamic = replace_options_for_dynamic(dynamic, type, args)

    if type == "pb_network":
        if "--search" not in dynamic:
            dynamic = ["--search"] + dynamic

        # Insert/Replace dynamic search configuration
        try:
            idx = args.search_options.index("--search")
            if len(args.search_options) == idx + 1:
                args.search_options[idx:] = dynamic
            else:
                if args.search_options[idx + 1] == "--":
                    args.search_options[idx:] = dynamic
                else:
                    raise ValueError("Cannot use dynamic search configuration with "
                                     "another given configuration.")
        except ValueError:
            args.search_options.extend(dynamic)
    else:
        assert False








