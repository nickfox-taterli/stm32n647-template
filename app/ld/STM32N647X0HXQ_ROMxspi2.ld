ENTRY(Reset_Handler)
_Min_Heap_Size = 0x200;
_Min_Stack_Size = 0x800;

_estack = ORIGIN(RAM) + LENGTH(RAM);
_sstack = _estack - _Min_Stack_Size;

MEMORY
{
  /*
   * XSPI2 NOR: physical 32MiB
   * Memory window: 0x70000000
   */
  /* Reserve the final 4 MiB (0x71C00000..0x71FFFFFF) for NPU weights. */
  ROM     (rx)  : ORIGIN = 0x70010000, LENGTH = 0x1BF0000

  /*
   * XSPI1 HyperRAM: physical 32MiB
   * Memory window: 0x90000000
   */
  HYPERRAM   (xrw) : ORIGIN = 0x90000000, LENGTH = 32M

  /*
   * Internal SRAM, secure aliases.
   */
  RAM        (xrw) : ORIGIN = 0x34000000, LENGTH = 1024K
  AXISRAM2   (xrw) : ORIGIN = 0x34100000, LENGTH = 1024K
  AXISRAM3   (xrw) : ORIGIN = 0x34200000, LENGTH = 448K
  AXISRAM4   (xrw) : ORIGIN = 0x34270000, LENGTH = 448K
  AXISRAM5   (xrw) : ORIGIN = 0x342E0000, LENGTH = 448K
  AXISRAM6   (xrw) : ORIGIN = 0x34350000, LENGTH = 448K

  /*
   * 条件可用区(默认不用):
   * STM32N6x7 上 CACHEAXI disabled 后可作 AXISRAM7;
   * VENCRAM/AXISRAM8 取决于 VENC 使用情况.
   */
  AXISRAM7   (xrw) : ORIGIN = 0x343C0000, LENGTH = 256K
  AXISRAM8   (xrw) : ORIGIN = 0x34400000, LENGTH = 128K

  /*
   * AHB SRAM / Backup SRAM
   */
  AHBSRAM1   (xrw) : ORIGIN = 0x38000000, LENGTH = 16K
  AHBSRAM2   (xrw) : ORIGIN = 0x38004000, LENGTH = 16K
  BKPSRAM    (xrw) : ORIGIN = 0x3C000000, LENGTH = 8K

  /*
   * TCM
   */
  ITCM       (xrw) : ORIGIN = 0x10000000, LENGTH = 64K
  DTCM       (xrw) : ORIGIN = 0x30000000, LENGTH = 128K
}

SECTIONS
{
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >ROM

  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)
    *(.RamFunc)
    *(.RamFunc*)
    KEEP (*(.init))
    KEEP (*(.fini))
    . = ALIGN(4);
    _etext = .;
  } >ROM

  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >ROM

  .shellCommand :
  {
    . = ALIGN(4);
    _shell_command_start = .;
    KEEP(*(shellCommand))
    . = ALIGN(4);
    _shell_command_end = .;
  } >ROM

  .ARM.extab   :
  {
    . = ALIGN(4);
    *(.ARM.extab* .gnu.linkonce.armextab.*)
    . = ALIGN(4);
  } >ROM

  .ARM :
  {
    . = ALIGN(4);
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
    . = ALIGN(4);
  } >ROM

  .preinit_array   :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
    . = ALIGN(4);
  } >ROM

  .init_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
    . = ALIGN(4);
  } >ROM

  .fini_array :
  {
    . = ALIGN(4);
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
    . = ALIGN(4);
  } >ROM

  _sidata = LOADADDR(.data);

  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    . = ALIGN(4);
    _edata = .;
  } >RAM AT> ROM

  .noncacheable :
  {
    . = ALIGN(8);
    __snoncacheable = .;
    KEEP(*(.noncacheable))
    . = ALIGN(8);
    __enoncacheable = .;
  } > RAM

  .gnu.sgstubs :
  {
    . = ALIGN(4);
    *(.gnu.sgstubs*)
    . = ALIGN(4);
  } >ROM

  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM

  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = . + _Min_Stack_Size;
    . = ALIGN(8);
  } >RAM

  .EXTRAM (NOLOAD) :
  {
    . = ALIGN(32);
    *(.EXTRAM)
    *(.EXTRAM*)
    . = ALIGN(32);
  } >HYPERRAM

  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
