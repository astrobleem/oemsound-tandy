"""Guarded native build; supply installed DOSBox-X, toolchain and shared lease."""
from pathlib import Path
import importlib.util
from importlib.machinery import SourceFileLoader

HERE = Path(__file__).resolve().parent
shared = (HERE / '../SHARED/BUILDAPP.PY').resolve()
spec = importlib.util.spec_from_file_location('consumer_build', shared,
    loader=SourceFileLoader('consumer_build', str(shared)))
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
module.build(HERE, 'TCHIME', ('TCHIME.C', 'TCHIME.DEF', 'TCHIME.RSP', 'BUILD.BAT'), (HERE / '../SHARED/PSGOWN.H').resolve())
