set pagination off
set confirm off
set remotetimeout 20

set $fsbl_sp = *(unsigned int *)0x34180400
set $fsbl_pc = *(unsigned int *)0x34180404
if $fsbl_sp < 0x34000000 || $fsbl_sp >= 0x38000000
  echo error: invalid RAM FSBL stack pointer\n
  quit 2
end
if $fsbl_pc < 0x34180401 || $fsbl_pc >= 0x34200000 || ($fsbl_pc & 1) == 0
  echo error: invalid RAM FSBL reset vector\n
  quit 2
end

set *(unsigned int *)0xE000ED08 = 0x34180400
set $msp = $fsbl_sp
set $pc = $fsbl_pc

# This symbol moves as FSBL changes, so resolve it from the current ELF.
hbreak JumpToApplication
continue

# Reaching JumpToApplication proves that FSBL initialized XSPI2. Refuse to
# resume if the mapped application vector is not plausible.
set $app_sp = *(unsigned int *)0x70010000
set $app_pc = *(unsigned int *)0x70010004
printf "APP vector verified: SP=0x%08x PC=0x%08x\n", $app_sp, $app_pc
if $app_sp < 0x34000000 || $app_sp >= 0x38000000
  echo error: invalid APP stack pointer\n
  quit 3
end
if $app_pc < 0x70010001 || $app_pc >= 0x72000000 || ($app_pc & 1) == 0
  echo error: invalid APP reset vector\n
  quit 3
end

delete breakpoints
# Detaching resumes at JumpToApplication; FSBL performs the actual VTOR/MSP/PC
# hand-off rather than having the launcher imitate it.
detach
