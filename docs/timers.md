# MechWarrior 2 Timer System Analysis

This document provides a detailed analysis of the timer system used in MechWarrior 2, based on reverse engineering of the game's code.

## Timer Frequency

- Both timers run at **181 Hz** (ticks per second)
- This frequency is set in the `FirstClock` function with the line: `_AIL_set_timer_divisor_8(TicksTimer,6556);`
- Each tick represents approximately 5.52 milliseconds of real time

## Timer Architecture

MechWarrior 2 uses a dual-timer system, with each timer serving different purposes in the game's architecture:

### Core Components

1. **Dual Timer Counters**
   - `Ticks1` and `Ticks2`: Two independent 32-bit counters that track game time
   - These counters are incremented by the `GameTickTimerCallback` function at 181Hz

2. **Timer Control Register**
   - `TicksCheck` (better name: `TimerPauseMask`): A global variable that controls which timer counters are paused
   - Bit 0x200 controls the `Ticks1` counter - when set, `Ticks1` is paused
   - Bit 0x100 controls the `Ticks2` counter - when set, `Ticks2` is paused

3. **Timer Arrays**
   - `ArrayOfTicks1Values` and `ArrayOfTicks2Values`: Arrays that store timestamp values for multiple independent timers
   - Each element in these arrays represents a different timer instance used by game objects or systems
   - The values stored are the tick counts when the timer was started or last updated

### Timer 1 (`Ticks1`) - Game Logic Timer

The first timer (controlled by bit 0x200 in `TimerPauseMask`) appears to be used for:

1. **Game State Management**
   - The `NextClock` function uses this timer to calculate `DeltaTime` between frames
   - This timer can be paused during game pauses and menu screens
   - Supports time compression (8x speed) and time expansion (1/4 speed) features

2. **Input Processing**
   - Used in `FUN_1003cc20` to implement cooldown periods between input actions
   - Prevents rapid-fire inputs by checking if enough time has elapsed (45 or 90 ticks)
   - Controls timing for joystick/controller input processing

3. **Game Mechanics**
   - Likely used for weapon cooldowns, movement physics, and other gameplay systems
   - The main timer that drives the game's simulation and logic

### Timer 2 (`Ticks2`) - Visual Effects Timer

The second timer (controlled by bit 0x100 in `TimerPauseMask`) is primarily used for:

1. **Animation Timing**
   - Used in `StartPalettes` to control palette transitions and visual effects
   - Animations run for specific durations (e.g., 271 ticks in the palette transition)

2. **Visual Effects**
   - Likely controls particle effects, explosion animations, and other visual elements
   - May continue running even when gameplay is paused

3. **UI Elements**
   - Probably handles timing for UI animations and transitions
   - Could control blinking elements, scrolling text, or other interface animations

## Key Functions

### Timer System Initialization

```c
void FirstClock(void)
{
  if (TicksTimerInitialized == FALSE) {
    _AIL_startup_0();
    TicksTimer = _AIL_register_timer_4(GameTickTimerCallback);
                    /* 181 Hz */
    _AIL_set_timer_divisor_8(TicksTimer,6556);
    _AIL_start_timer_4(TicksTimer);
    // ... initialize timer slots ...
    TicksTimerInitialized = TRUE;
  }
  // ... reset clock values ...
}
```

### Timer Callback

```c
void GameTickTimerCallback(void)
{
  if ((TicksCheck & 0x200) == 0) {
    Ticks1 = Ticks1 + 1;
  }
  if ((TicksCheck & 0x100) == 0) {
    Ticks2 = Ticks2 + 1;
  }
  return;
}
```

### Timer Control

```c
void __cdecl _pause_timer(ushort timerSelector, BOOL pauseState)
{
  uint mappedTimerBits = 0;
  
  if ((timerSelector & 0x80) != 0) {
    mappedTimerBits = 0x200;
  }
  if ((timerSelector & 0x100) != 0) {
    mappedTimerBits = mappedTimerBits | 0x100;
  }
  if ((short)pauseState == FALSE) {
    TicksCheck = TicksCheck & ~mappedTimerBits;
  }
  else {
    TicksCheck = TicksCheck | mappedTimerBits;
  }
  return;
}
```

### Timer Slot Allocation

```c
longlong FUN_10067f01(uint param_1)
{
  int iVar1;
  uint uVar2;
  undefined4 in_EDX;
  int *ticks2Value;
  
  if ((param_1 & 0x80) == 0) {
    uVar2 = 0;
    for (ticks2Value = &ArrayOfTicks2Values; *ticks2Value != 0; ticks2Value = ticks2Value + 1) {
      if (*ticks2Value == -1) goto LAB_10067f64;
      uVar2 = uVar2 + 1;
    }
    iVar1 = Ticks2;
    if (Ticks2 == 0) {
      iVar1 = 1;
    }
    *ticks2Value = iVar1;
  }
  else {
    uVar2 = 0;
    for (ticks2Value = &ArrayOfTicks1Values; *ticks2Value != 0; ticks2Value = ticks2Value + 1) {
      if (*ticks2Value == -1) goto LAB_10067f64;
      uVar2 = uVar2 + 1;
    }
    iVar1 = Ticks1;
    if (Ticks1 == 0) {
      iVar1 = 1;
    }
    *ticks2Value = iVar1;
    uVar2 = uVar2 | 0x80;
  }
LAB_10067f68:
  return CONCAT44(in_EDX,uVar2);
LAB_10067f64:
  uVar2 = CONCAT22((short)(param_1 >> 0x10),0xffff);
  goto LAB_10067f68;
}
```

### Elapsed Time Calculation

```c
int __cdecl FUN_10067f6f(uint param_1)
{
  int iVar1;
  undefined4 *puVar2;
  
  if ((param_1 & 0x80) == 0) {
    puVar2 = &ArrayOfTicks2Values;
    iVar1 = Ticks2;
  }
  else {
    param_1 = param_1 ^ 0x80;
    puVar2 = &ArrayOfTicks1Values;
    iVar1 = Ticks1;
  }
  return iVar1 - puVar2[param_1];
}
```

### Timer Value Update

```c
void __cdecl UpdateArraysOfTicksValues(uint param_1)
{
  undefined4 uVar1;
  undefined4 *puVar2;
  
  if ((param_1 & 0x80) == 0) {
    puVar2 = &ArrayOfTicks2Values;
    uVar1 = Ticks2;
  }
  else {
    param_1 = param_1 ^ 0x80;
    puVar2 = &ArrayOfTicks1Values;
    uVar1 = Ticks1;
  }
  puVar2[param_1] = uVar1;
  return;
}
```

### Frame Time Calculation

```c
void NextClock(void)
{
  if (TicksTimerInitialized != 0) {
    if (DAT_100ba554 == 0) {
      CurrentClock = FUN_10067f6f(DAT_100ba56c);
      if (0 < FramerateLimit) {
        while (CurrentClock - PreviousClock < FramerateLimit) {
          CurrentClock = FUN_10067f6f(DAT_100ba56c);
        }
      }
      // ... handle pausing ...
    }
    // ... handle unpausing ...
    DeltaTime = CurrentClock - PreviousClock;
    if (IsNetworkGame == 0) {
      if (TimeCompressionEnabled == 0) {
        if (TimeExpansionEnabled != 0) {
          DeltaTime = DeltaTime >> 2;  // 1/4 speed
          CurrentClock = CurrentClock + DeltaTime;
          FUN_10067fe5(DAT_100ba56c,CurrentClock);
        }
      }
      else {
        DeltaTime = DeltaTime * 8;  // 8x speed
        CurrentClock = CurrentClock + DeltaTime;
        FUN_10067fe5(DAT_100ba56c,CurrentClock);
      }
    }
    // ... handle minimum delta time ...
    PreviousClock = CurrentClock;
  }
  return;
}
```

## System Operation

1. **Timer Initialization**
   - The game initializes the timer system in `FirstClock`, setting up the arrays and starting the timer callback

2. **Timer Allocation**
   - Game systems request timer slots using `FUN_10067f01`
   - Each game object or system that needs timing gets its own slot in one of the arrays

3. **Time Tracking**
   - The `GameTickTimerCallback` continuously increments the tick counters
   - Game code can calculate elapsed time by comparing current tick values with stored timestamps

4. **Timer Control**
   - The game can pause specific timer subsystems using `_pause_timer`
   - This allows the game to pause time-dependent systems (like animations or physics) while keeping others running

## Applications in the Game

This dual-timer architecture provides MechWarrior 2 with a flexible timing system that separates gameplay logic from visual effects, allowing for more sophisticated control over the game's pacing and presentation. The high frequency (181 Hz) ensures smooth gameplay and precise timing for both the simulation and visual elements.

The timer system likely supports various game features:

1. **Animation Timing**: Controlling the speed and synchronization of mech animations
2. **Physics Simulation**: Maintaining consistent physics calculations regardless of frame rate
3. **Game Events**: Triggering time-based events like mission objectives or enemy spawns
4. **UI Elements**: Managing timed UI elements like message displays or alerts
5. **Pause Functionality**: Supporting the game's pause feature while allowing certain systems to continue running

The dual-timer design suggests the game separates different types of timing - perhaps one for gameplay logic and another for visual elements or real-time features that should continue even when gameplay is paused.