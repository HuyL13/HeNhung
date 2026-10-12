"""Compile real C logic for Cortex-M4 and execute tests with Unicorn ARM."""
from pathlib import Path
import subprocess, sys
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT.parent / 'lab1_review/test_tools'))
try:
    from unicorn import Uc, UC_ARCH_ARM, UC_MODE_THUMB, UC_MODE_MCLASS
    from unicorn.arm_const import UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_R0
except ImportError:
    raise SystemExit('Install test dependency: python -m pip install unicorn')
gcc = next(Path('C:/ST/STM32CubeIDE_2.0.0').rglob('arm-none-eabi-gcc.exe'))
OUT = ROOT / 'tests/build'; OUT.mkdir(exist_ok=True)
flags = ['-mcpu=cortex-m4', '-mthumb', '-O0', '-g', '-ffreestanding', '-fno-builtin', '-nostdlib',
         '-I'+str(ROOT/'Core/Inc'), '-Wl,-Ttext=0x10000', '-Wl,--entry=test_nec_valid']
if '--red' not in sys.argv: flags += ['-DFULL_TESTS']
elf = OUT/'logic.elf'; binary = OUT/'logic.bin'
subprocess.run([str(gcc), *flags, str(ROOT/'Core/Src/fusion_logic.c'),
                str(ROOT/'tests/logic_cases.c'), '-o', str(elf)], check=True)
subprocess.run([str(gcc.with_name('arm-none-eabi-objcopy.exe')), '-O', 'binary', str(elf), str(binary)], check=True)
nm = subprocess.check_output([str(gcc.with_name('arm-none-eabi-nm.exe')), str(elf)], text=True)
tests = [(int(line.split()[0],16),line.split()[2]) for line in nm.splitlines()
         if len(line.split())==3 and line.split()[2].startswith('test_')]
for address, name in tests:
    arm = Uc(UC_ARCH_ARM, UC_MODE_THUMB | UC_MODE_MCLASS)
    arm.mem_map(0x10000, 0x10000); arm.mem_write(0x10000, binary.read_bytes())
    arm.mem_map(0x20000000, 0x10000)
    arm.reg_write(UC_ARM_REG_SP, 0x2000FFF0); arm.reg_write(UC_ARM_REG_LR, 0x1F001)
    arm.emu_start(address|1, 0x1F000, count=2000000)
    result = arm.reg_read(UC_ARM_REG_R0)
    if result:
        raise SystemExit(f'FAIL {name}: check at C line {result}')
    print('PASS', name)
print(f'{len(tests)} ARM logic tests passed.')
