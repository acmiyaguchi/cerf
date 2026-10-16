# LEDs and LED buttons

An LED is a host widget in the status bar. It is implemented as a part of
device's user physical experience. LEDs can display debugging data from guest,
or important notifications data - for user.

## The shape

The peripheral or board service that owns the LED registers also implements
`HostWidget`. It registers itself with `HostWidgetRegistry` in `OnReady`, and
`Group()` returns `WidgetGroup::Indicator`.

- **Describe the LED, never draw it.** `Icon()` returns a `LedIcon`. The fill
  carries lit or dark. The rim carries a second bit, usually blink mode.
- **Repaint through `PollDirty`.** An LED carries no data path, so `MarkRx`
  and `MarkTx` never fire for it. Keep the drawn appearance in members that
  only the UI thread touches.
- **Compute the blink phase in `PollDirty`.** Count ticks in the widget, or read
  `HostSteadyMicros()`.
- **Read the state under the lock.** Then build the icon from that copy,
  outside the lock. The guest writes the registers on the JIT thread, and the UI thread
  reads them.
- **Serialize the registers only.** The blink tick and the last-drawn members
  belong to the UI thread.

## Variants

- **On-chip LED unit.** `libCerfCore/src/socs/vr41xx/vr41xx_led_impl.h` - the VR41xx unit
  blinks in hardware, so the registers hold the on-time and the off-time.
  `PollDirty` converts those units into its own ticks.
- **LED behind a companion MCU.** `libCerfCore/src/boards/hp_jornada_700/hp_jornada_700_led.h` -
  the LED has no MMIO of its own. The class is a plain `Service`, and the MCU
  command handler calls `SetState()` from the JIT thread.
- **One block that drives more than an LED.**
  `libCerfCore/src/boards/samsung_smdk2410/samsung_smdk2410_nled.cpp` - two channels, where the
  second one is a vibrator. One `LedIcon` carries both: the LED, and the
  vibrator as its `vibrate` flag.
- **A row of LEDs.**
  `libCerfCore/src/boards/odo/odo_hkeep_fpga.cpp` - one widget holds eight
  discrete debug LEDs in one `LedGridIcon`. The tooltip carries the
  four-character alpha display of the same block. Combine the LEDs of one
  block into one widget.

## Pressable LED buttons

Some boards put a button in the LED. The LED and its button are one widget.

- The left click presses the button.
- The menu offers the same press as a named item.
- **The press drives the pin that the board wires.** The Jornada 720 button is
  GPIO 13. `OnReady` drives that input to its idle level, and the press drives
  the edge. The guest driver turns that edge into an interrupt.
