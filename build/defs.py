from enum import Enum

class OS(Enum):
    Windows = 1
    macOS = 2
    Linux = 3

class Baseline(Enum):
    i386 = 1
    i486 = 2
    i586 = 3
    i686 = 4

class Architecture(Enum):
    x86 = 1

architecture_strings: dict[str, Architecture] = {
    "x86": Architecture.x86
}

baseline_strings: dict[str, Baseline] = {
    "i386": Baseline.i386,
    "i486": Baseline.i486,
    "i586": Baseline.i586,
    "i686": Baseline.i686
}

baseline_architectures: dict[Baseline, Architecture] = {
    Baseline.i386: Architecture.x86,
    Baseline.i486: Architecture.x86,
    Baseline.i586: Architecture.x86,
    Baseline.i686: Architecture.x86
}

architecture_baselines: dict[Architecture, Baseline] = {
    Architecture.x86: Baseline.i386
}
