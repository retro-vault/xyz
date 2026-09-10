#!/usr/bin/env python3
"""Check AST constant values using Python integers and explicit target ranges."""
import itertools
import pathlib
import subprocess
import sys

MASK64 = (1 << 64) - 1
STANDARD = {
    "bool": (1, False), "schar": (8, True), "uchar": (8, False),
    "short": (16, True), "ushort": (16, False),
    "int": (16, True), "uint": (16, False),
    "long": (32, True), "ulong": (32, False),
    "llong": (64, True), "ullong": (64, False),
}


def details(name):
    return STANDARD[name] if name in STANDARD else (int(name[1:]), name[0] == "s")


def limits(name):
    width, signed = details(name)
    return (-(1 << (width - 1)), (1 << (width - 1)) - 1) if signed else (0, (1 << width) - 1)


def normalize(value, name):
    if name == "bool":
        return int(value != 0)
    width, signed = details(name)
    value %= 1 << width
    return value - (1 << width) if signed and value >= 1 << (width - 1) else value


def promote(name):
    if name in ("bool", "schar", "uchar", "short", "ushort"):
        low, high = limits(name)
        return "int" if -32768 <= low and high <= 32767 else "uint"
    return name


def rank(name):
    width, _ = details(name)
    # Insert standard types after bit-precise types of the same width;
    # short and int remain distinct ranks on this target.
    return width * 4 + (2 if name in ("int", "uint") else 1 if name in STANDARD else 0)


def common(a, b):
    a, b = promote(a), promote(b)
    if details(a)[1] == details(b)[1]:
        return max((a, b), key=rank)
    signed, unsigned = (a, b) if details(a)[1] else (b, a)
    if rank(unsigned) >= rank(signed):
        return unsigned
    if limits(signed)[1] >= limits(unsigned)[1]:
        return signed
    return {"int": "uint", "long": "ulong", "llong": "ullong"}.get(signed, "u" + signed[1:])


def calculate(op, a, av, b, bv):
    av, bv = normalize(av, a), normalize(bv, b)
    result_type = promote(a) if op in ("shl", "shr", "neg", "bnot") else common(a, b)
    if op in ("land", "lor", "not"):
        result = (bool(av) and bool(bv)) if op == "land" else (bool(av) or bool(bv)) if op == "lor" else not av
        return "int", int(result)
    if op in ("cond0", "cond1"):
        return result_type, normalize(av if op == "cond1" else bv, result_type)
    x = normalize(av, result_type)
    y = normalize(bv, promote(b) if op in ("shl", "shr") else result_type)
    if op in ("eq", "ne", "lt", "le", "gt", "ge"):
        return "int", int({"eq": x == y, "ne": x != y, "lt": x < y,
                           "le": x <= y, "gt": x > y, "ge": x >= y}[op])
    if op in ("shl", "shr"):
        if y < 0 or y >= details(result_type)[0]:
            return None  # Undefined shift counts are not oracle cases.
        if op == "shl" and details(result_type)[1] and (x < 0 or x << y > limits(result_type)[1]):
            return None  # Undefined signed left shift is not an oracle case.
        z = x << y if op == "shl" else x >> y
    elif op in ("div", "mod"):
        if y == 0:
            return None  # Division by zero is not a defined constant value.
        if details(result_type)[1] and x == limits(result_type)[0] and y == -1:
            return None  # Undefined signed division overflow.
        quotient = abs(x) // abs(y) * (-1 if (x < 0) != (y < 0) else 1)
        z = quotient if op == "div" else x - quotient * y
    elif op == "neg":
        z = -x
    elif op == "bnot":
        z = ~x
    else:
        z = {"add": lambda: x + y, "sub": lambda: x - y, "mul": lambda: x * y,
             "and": lambda: x & y, "or": lambda: x | y, "xor": lambda: x ^ y}[op]()
    if op in ("add", "sub", "mul", "neg") and details(result_type)[1] and not limits(result_type)[0] <= z <= limits(result_type)[1]:
        return None  # Never compare compiler behavior for signed overflow.
    return result_type, normalize(z, result_type)


widths = (1, 2, 3, 7, 8, 9, 15, 16, 17, 31, 32, 33, 63, 64)
names = list(STANDARD) + [sign + str(n) for n in widths for sign in ("s", "u") if n > 1 or sign == "u"]
operations = ("add", "sub", "mul", "div", "mod", "and", "or", "xor", "shl", "shr",
              "eq", "ne", "lt", "le", "gt", "ge", "land", "lor", "cond0", "cond1")
cases = []


def add(op, a, av, b, bv):
    calculated = calculate(op, a, av, b, bv)
    if calculated is None:
        return
    result_type, result = calculated
    line = f"{op} {a} {av & MASK64} {b} {bv & MASK64} {result_type}\n"
    cases.append((line, "none" if result is None else str(result & MASK64)))


for a, b in itertools.product(names, repeat=2):
    if a in STANDARD and b in STANDARD:
        continue  # This repair changes only expressions involving BitInt.
    alo, ahi = limits(a)
    blo, bhi = limits(b)
    for av, bv in ((0, 1), (1, 0), (alo, bhi), (ahi, blo), (ahi, 2), (alo, -1), (-1, bhi)):
        for op in operations:
            add(op, a, av, b, bv)

# Every supported width, including widths between storage boundaries.
for width in range(1, 65):
    for sign in ("s", "u"):
        if width == 1 and sign == "s":
            continue
        a = sign + str(width)
        low, high = limits(a)
        for raw in (0, 1, -1, low, high, high - 1):
            for op in ("neg", "bnot", "not"):
                add(op, a, raw, "int", 0)
            for count in (0, width - 1, width, -1):
                for op in ("shl", "shr"):
                    add(op, a, raw, "int", count)
            for op in ("add", "sub", "mul", "div", "mod"):
                add(op, a, raw, a, 1)

program = pathlib.Path(sys.argv[1]).resolve()
result = subprocess.run([str(program)], input="".join(line for line, _ in cases),
                        capture_output=True, text=True, check=True)
rows = result.stdout.splitlines()
assert len(rows) == len(cases), (len(rows), len(cases), result.stderr)
for (line, expected), actual in zip(cases, rows):
    assert actual == expected, (line.strip(), expected, actual)
print(len(cases), "independent BitInt constant-value checks passed")
