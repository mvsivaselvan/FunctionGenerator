# Function Generator State Machines

This document maps the two finite state machines implemented in
`RampAndFunctionGenerator.c` / `RampAndFunctionGenerator.h` to their
original design sketch. The code was originally developed as an
NI VeriStand simulation model; the VeriStand-specific wrapper code
(in `user_code.c` and the `ni_modelframework` interface) is orthogonal
to the state machines themselves and is not covered here.

## 1. Overview

The function generator produces multi-channel periodic signals
(sine, sawtooth, smooth sawtooth, square) with smoothly ramped
amplitude and frequency. Two cooperating state machines implement
this:

1. A top-level **Function Generator FSM** that controls the overall
   start / run / stop lifecycle.
2. A reusable **Ramp Generator FSM** that smoothly transitions a
   vector of values from a current value to a new commanded value
   over a configurable ramp time. Two independent instances of the
   ramp generator exist inside the function generator: one for
   amplitude, one for frequency.

The number of channels `NCHAN` and the sample period `SAMPLE_DT` are
defined at compile time (typically 4 channels at 1 ms in the original
build; the QNX target will use SAMPLE_DT = 1.0/1024.0).

## 2. Function Generator FSM

### 2.1 States

The states are defined by `enum FunctionGeneratorState` in
`RampAndFunctionGenerator.h`:

- `FSidle`     -- no signal generation; output equals offset.
- `FSready`    -- a one-step transient state used to prime the
                  amplitude ramp generator on the first cycle of a
                  start command. Not shown on the original sketch.
- `FSrunning`  -- generating output; ramps respond to live changes
                  in amplitude and frequency commands.
- `FSstopping` -- a stop command was received; amplitude is ramping
                  down to zero. The generator returns to `FSidle`
                  when the amplitude ramp completes.

### 2.2 Per-state context

While running, the FSM tracks (per channel):

- `phase[NCHAN]`  -- the running phase argument fed to the waveform
                     function. Wraps modulo 2*PI.
- `program_source` -- which waveform is selected (sine, sawtooth,
                     smooth sawtooth, square, or external).

### 2.3 Transitions

| From        | Event                                  | To          | Action                                                          |
|-------------|----------------------------------------|-------------|-----------------------------------------------------------------|
| FSidle      | start && source != PSnone, source == PSexternal | FSrunning  | Latch program source; latch ramp times.                          |
| FSidle      | start && source != PSnone, source != PSexternal | FSready    | Latch program source; latch ramp times.                          |
| FSidle      | (no start) or source == PSnone         | FSidle      | Output equals offset on every channel.                          |
| FSready     | (unconditional, next cycle)            | FSrunning   | Prime amplitude ramp generator with commanded amplitude.        |
| FSrunning   | stop && program_source == PSexternal   | FSidle      | Drop straight to idle (no amplitude ramp for external mode).    |
| FSrunning   | stop && program_source != PSexternal   | FSstopping  | Latch ramp times; command amplitude ramp generator toward zero. |
| FSrunning   | (no stop)                              | FSrunning   | If not external: re-issue ramp generator with current amp/freq commands so changes are tracked. |
| FSstopping  | amplitude ramp generator == RSidle     | FSidle      | Clear program source; output equals offset.                     |
| FSstopping  | otherwise                              | FSstopping  | Continue commanding amplitude ramp generator toward zero.       |

### 2.4 Mapping to the sketch

The original sketch shows three states (idle, running, stopping)
with these transitions:

- idle -> running on `start` (set program source, start ramping up
  amplitude)
- running -> stopping on `stop` (start ramping down amplitude)
- stopping -> idle on `done`

The implementation matches this directly, with two refinements:

- `FSready` is a one-step intermediate between `FSidle` and
  `FSrunning` whose only job is to make the first call to the
  amplitude ramp generator with the commanded amplitude before any
  output is produced. From the user's point of view the start
  transition behaves as drawn.
- `PSexternal` -- not present on the sketch -- is a source mode
  that bypasses the internal oscillator. In this mode the output
  is simply `externalIn + offset`, no amplitude or frequency ramps
  are applied, and stop returns directly to `FSidle`.

### 2.5 Output equation while running

For internal sources (`PSsine`, `PSsawtooth`, `PSsmooth_sawtooth`,
`PSsquare`), each channel produces:

    out[n] = ampl_factor[n] * FunctionGenerator(source, phase[n])
             + offset_in[n]
    phase[n] += 2 * PI * frequency[n] / sampling_freq
    if phase[n] >= 2 * PI: phase[n] -= 2 * PI

where `ampl_factor[n]` and `frequency[n]` are the (possibly ramping)
outputs of the two ramp generator instances.

For `PSexternal`:

    out[n] = externalIn[n] + offset_in[n]

## 3. Ramp Generator FSM

### 3.1 States

Defined by `enum RampingState`:

- `RSidle`    -- holding the last reached value; output is constant.
- `RSramping` -- output is interpolating from `begin_val` to
                 `end_val` along a smooth profile.

### 3.2 Per-state context (per instance)

`struct RampGenState` in `RampAndFunctionGenerator.h`:

- `begin_val[NCHAN]`   -- value at the start of the current ramp.
- `end_val[NCHAN]`     -- target value the ramp is moving toward.
- `current_val[NCHAN]` -- the value emitted on the previous step;
                          this is also the output `out[]`.
- `ramp_fract`         -- progress along the current ramp,
                          nominally 0.0 at `begin_val` and 1.0 at
                          `end_val`.
- `ramp_time`          -- duration of the current ramp in seconds.

### 3.3 Transitions

The ramp generator is called on every step with a commanded value
vector `val[]` and a `ramp_time`. It detects a change-value event by
comparing each element of `val[]` to `end_val[]` with a tolerance
of 1e-6.

| From      | Event             | To        | Action                                                          |
|-----------|-------------------|-----------|-----------------------------------------------------------------|
| RSidle    | change_val        | RSramping | Set `begin_val = current_val`, set `end_val = val`, set `ramp_fract = 1 / (ramp_time * sampling_freq)`, latch `ramp_time`. |
| RSidle    | no change_val     | RSidle    | Hold `current_val`.                                             |
| RSramping | change_val        | RSramping | Same actions as start (re-target the ramp from the current value). |
| RSramping | no change_val     | RSramping | Advance `ramp_fract`; if it reaches 1.0, snap `current_val = end_val` and go to `RSidle`. |

### 3.4 Mapping to the sketch

The sketch shows:

- idle -> ramping on `start / change_val` (set begin_val, end_val,
  ramp_fract = 0, ramp_time = input ramp time)
- ramping -> ramping (self-loop) on `change_val` (re-set begin_val,
  end_val, ramp_fract = 0, ramp_time = input ramp time)
- ramping -> idle on `done_ramping`

The implementation matches this, with one detail: when a change
is detected, `ramp_fract` is set to `1 / (ramp_time * sampling_freq)`
rather than to exactly 0.0. This is because the increment that would
have been added on the first step is folded into the initial value,
which keeps the increment formula uniform and avoids emitting a
duplicate sample at `begin_val`.

### 3.5 Output equation

On every call:

    if ramping_state == RSramping:
        ramp_val = get_ramp_val(ramp_fract)
        for each channel n:
            current_val[n] = begin_val[n]
                           + (end_val[n] - begin_val[n]) * ramp_val
    out[n] = current_val[n]

`get_ramp_val(u)` is a smoothed clamp: it equals 0 for u <= 0,
equals 1 for u >= 1 + DELT, and uses the polynomial blend
`smooth_trans(x, DELT)` from `RampAndFunctionGenerator.c` near both
endpoints to give continuous derivatives.

### 3.6 Two instances inside the function generator

Two independent `RampGenState` structures live inside
`RampFuncGenState`:

- `ampl_rampgen_state` -- driven from the commanded amplitudes;
                          its output multiplies the waveform.
- `freq_rampgen_state` -- driven from the commanded frequencies;
                          its output is the per-channel angular
                          velocity (after multiplying by 2*PI / Fs).

The amplitude ramp is also commanded toward the zero vector by the
function generator FSM during `FSstopping`, which is how a clean
fade-out is implemented.

## 4. Source-to-state cross reference

| Code element                                    | Role in state machine                          |
|-------------------------------------------------|------------------------------------------------|
| `enum FunctionGeneratorState`                   | Top-level FSM state enum.                      |
| `enum RampingState`                             | Ramp FSM state enum.                           |
| `enum ProgramSource`                            | Selects the waveform; PSexternal bypasses ramps and oscillator. |
| `enum StartStop`                                | UI command into the top-level FSM.             |
| `struct RampFuncGenState`                       | Top-level FSM context plus the two ramp instances. |
| `struct RampGenState`                           | Ramp FSM context.                              |
| `RampAndFunctionGeneratorInit()`                | Initializes both FSMs to idle and zeroes phase / ramp values. |
| `RampAndFunctionGeneratorExec(...)`             | Runs the top-level FSM for one sample step.    |
| `RampGenerator(...)`                            | Runs one ramp FSM instance for one sample step. |
| `FunctionGenerator(source, phase)`              | Pure waveform lookup; no state.                |
| `get_ramp_val(ramp_fract)`                      | Smoothed 0-to-1 ramp profile with C1 endpoints. |
| `smooth_trans(x, delt)`, `smooth_trans2(x, delt)` | Polynomial blends used by `get_ramp_val` and `smooth_sawtoothfunc`. |
| `sinefunc`, `sawtoothfunc`, `smooth_sawtoothfunc`, `squarefunc` | Stateless waveform shapes. |
| `_state` (file-scope)                           | The single instance of `RampFuncGenState`.     |

## 5. Build-time configuration

- `NCHAN`     -- number of independent channels (compile-time).
- `SAMPLE_DT` -- sample period in seconds (compile-time). The FSM
                 uses `1.0 / SAMPLE_DT` as `sampling_freq` when
                 computing ramp increments and phase increments.
- `DELT`      -- smoothing parameter for the ramp endpoints and
                 the smooth sawtooth peak. Defined as 0.1 in the
                 header.

## 6. Notes for non-VeriStand integration

The state machines and waveform code are self-contained: they
depend only on `<math.h>`, `<stdlib.h>`, the macros above, and a
single file-scope `_state` instance. To embed in another runtime
(for example, the QNX three-thread architecture), the runtime
needs to:

1. Define `NCHAN` and `SAMPLE_DT` at compile time.
2. Call `RampAndFunctionGeneratorInit()` once.
3. Call `RampAndFunctionGeneratorExec(...)` once per sample period
   from the time-critical context, passing in the current commanded
   source, start_stop, ramp times, frequency vector, amplitude
   vector, external input vector, offset vector, and an output
   buffer.
4. Call `RampAndFunctionGeneratorFinal()` at shutdown (currently a
   no-op, but kept for symmetry).

The file-scope `_state` makes the module non-reentrant; only one
function generator instance can exist per process linkage unit.
