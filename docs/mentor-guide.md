# Understanding equipment pulse recorder

A rising edge interrupts the processor briefly. The interrupt accepts an edge only if it is far enough from the previous accepted edge, then updates a count and timestamp. Every 100 ms the task snapshots that shared state. Speed is the count difference divided by actual elapsed time and pulses per revolution. A saturating idle timer prevents a stopped machine from appearing active when the microsecond clock wraps.

## Code walkthrough

### src/main.cpp: pulse

Read the timestamp outside the lock, then update only count/time state inside it. Explain why the output task must take the same lock.

### include/recorder.hpp: Recorder.update

Walk through dc, dt, RPM and idle-age saturation. Explain the less-than-one-wrap assumption.

### include/recorder.hpp: Config

Show magic, version, PPR range and checksum validation. A stored integer alone would lack these checks.

### src/main.cpp: loop

Follow bounded command parsing, delay-until scheduling, snapshot, calculations, LED and CSV.

### tests/test.cpp: longidle

Describe the original regression and why 90,000 synthetic 100 ms intervals test two timer wraps in seconds of host time.

### tools/simulate.cpp

Compare fixed 100 ms division against measured intervals on the identical pulse trace.

## Ten interview questions

### 1. Why use actual elapsed time?

A task may run early or late. Dividing counts by an assumed 100 ms produces a speed error proportional to interval error.

### 2. Why does unsigned subtraction handle rollover?

Arithmetic is modulo 2^32. If the actual elapsed interval is less than one wrap, now minus then gives that interval even when the counter crossed zero.

### 3. Why was subtraction alone insufficient for stall detection?

The last edge can be older than a complete wrap. Its modular age then looks small again. Saturating accumulated idle age avoids that ambiguity.

### 4. Does volatile make shared data safe?

No. It prevents certain compiler assumptions but does not make a multi-field snapshot atomic. The ISR and task use the same critical-section lock.

### 5. What limits the maximum input frequency?

Debounce rejects edges closer than 500 microseconds, and ISR service also consumes time. The design targets 1 kHz and requires physical verification.

### 6. What does PPR mean?

Pulses per revolution. RPM equals pulses per second times 60 divided by PPR. A wrong PPR produces a proportional calibration error.

### 7. Why does low-speed RPM jump?

A short window may contain zero or one pulse. Period measurement or adaptive windows can improve low-speed resolution at the cost of response behavior.

### 8. Why is runtime approximate?

The state remains active until the two-second stall timeout and updates in reporting intervals. The metric includes that tail and boundary quantization.

### 9. What is the ADC measuring?

Only raw counts from a 0-3.3 V test input. Reference variation and nonlinearity mean a voltage claim needs calibration.

### 10. How would you improve the hardware?

Use the ESP32 pulse-counter peripheral for higher rates, an appropriate input protection/level interface, and physical timing tests. Compare PCNT and ISR results before claiming improvement.

## Five minute demonstration

1. Run sh tools/test.sh; point out the long-idle regression and malformed-command checks.
2. Run sh tools/simulate.sh and inspect the jitter comparison chart.
3. Show why 80 ms and 120 ms intervals fool the baseline but not elapsed-time normalization.
4. Follow the stopped trace to STALLED, then state that real jitter and pulse-loss claims await a generator.

## Independent exercises

- Implement reciprocal-period RPM for low rates and test transition behavior.
- Add a PCNT hardware adapter and compare missed pulses and CPU cost on a board.
- Separate exact pulse-present runtime from the current stall-qualified runtime and test both semantics.

## Resume wording and contribution

These describe the implemented artifact and recorded tests. The code was generated with AI assistance in this session; do not present it as independently authored or physically validated. Use these bullets only after you can reproduce and explain the implementation, and retain the AI-assisted description where it accurately reflects your contribution.

- Created an AI-assisted C++ equipment recorder with bounded pulse interrupts, configurable pulses per revolution, stall indication and persistent configuration; cross-compiled for ESP32.
- Fixed a timer-rollover stall defect and validated stopped-state persistence across 90,000 synthetic reporting intervals.
- Eliminated fixed-window normalization error on a deterministic jitter trace by calculating RPM from actual elapsed time; physical timing remains unmeasured.

For a later hardware achievement, first execute the numbered experiments in hardware.md, retain raw captures and revise the bullet with the actual sample count and measured result. No physical percentage, battery lifetime, throughput or accuracy is supplied as a placeholder.
