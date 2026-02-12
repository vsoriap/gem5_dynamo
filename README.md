# The DynAMO Predictor in gem5 Simulator

This is the repository for DynAMO an AMO predictor for the L1D integrated within the gem5 simulator.

The gem5 simulator main website can be found at <http://www.gem5.org>.

## Getting started

DynAMO works under the CHI protocol with ARM and RISCV ISAs. DynAMO uses the ProxyPrefetch approach to interface with the CHI Cache Controller. There is a new class RubyPredictor in src/mem/ruby/structures. Ths class must be instantiated when configuring the L1D CHI Cache Controller (config/ruby/CHI_config.py). There are some parameters that can be configured when instantiating the predictor (Number of entries, Associativity Max Predictor Counter, Initial Counter Value). Once DynAMO is instantiated, the L1D Cache Controller calls DynAMO to obtain predictions and notify events. 

## Referencing this repo

You can reference this repo by citing the original ["ISCA'23"](https://doi.org/10.1145/3579371.3589065) paper:

```
@inproceedings{10.1145/3579371.3589065,
author = {Soria-Pardos, V\'{\i}ctor and Armejach, Adri\`{a} and M\"{u}ck, Tiago and Su\'{a}rez-Gracia, Dario and Joao, Jos\'{e} and Rico, Alejandro and Moret\'{o}, Miquel},
title = {DynAMO: Improving Parallelism Through Dynamic Placement of Atomic Memory Operations},
year = {2023},
isbn = {9798400700958},
publisher = {Association for Computing Machinery},
address = {New York, NY, USA},
url = {https://doi.org/10.1145/3579371.3589065},
doi = {10.1145/3579371.3589065},
booktitle = {Proceedings of the 50th Annual International Symposium on Computer Architecture},
articleno = {30},
numpages = {13},
keywords = {data placement, atomic memory operations, microarchitecture, multi-core architectures},
location = {Orlando, FL, USA},
series = {ISCA '23}
}
```
