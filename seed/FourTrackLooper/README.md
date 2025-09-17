# Four-Track Looper for Daisy Seed

A multi-track audio looper inspired by the Critter & Guitari Five Moons, designed for the Daisy Seed platform. This looper provides 4 independent tracks that can record, loop, and playback audio with various recording modes.

## Available Versions

This project includes three different implementations to suit different needs and complexity levels:

### 1. FourTrackLooper.cpp (Standard Version)
- 4 independent tracks with individual record buttons
- Mode selection buttons for each track
- Volume controls via potentiometers
- Master record and clear all functions
- LED status feedback
- Moderate pin count (13 digital + 5 analog)

### 2. FourTrackLooperAdvanced.cpp (Full-Featured Version)
- All standard features plus:
- Individual mute/solo buttons per track
- Reverse and half-speed controls per track
- Pitch shifting, filtering, and feedback effects per track
- Stereo panning controls
- Performance mode with crossfading
- Sync/tempo controls
- Complex LED patterns
- High pin count (22+ pins required)

### 3. FourTrackLooperSimple.cpp (Minimal Version)
- Essential 4-track functionality
- Simple control scheme (6 buttons + 5 knobs)
- Track selection via last-pressed record button
- Single mode switch affects selected track
- Double-tap record buttons to clear individual tracks
- Easiest to wire and understand
- Low pin count (6 digital + 5 analog)

## Features

- **4 Independent Tracks**: Each track can record up to 30 seconds of audio
- **Multiple Recording Modes**: Normal, One-time Dub, Replace, and Frippertronics modes
- **Real-time Control**: Individual record buttons and mode selection for each track
- **Master Controls**: Master record button and clear all tracks functionality
- **LED Feedback**: Visual indication of recording status and track activity
- **Solo/Mute**: Individual track control (expandable)
- **Dry Signal Monitoring**: Low-level input monitoring for recording reference

## Hardware Requirements

### Daisy Seed
- Daisy Seed development board
- Audio input/output connections
- Breadboard or custom PCB for button connections

### Pin Connections

#### Simple Version (FourTrackLooperSimple.cpp)
| Function | Pin | Description |
|----------|-----|-------------|
| Track 1 Record | D15 | Record/Stop button for track 1 |
| Track 2 Record | D16 | Record/Stop button for track 2 |
| Track 3 Record | D17 | Record/Stop button for track 3 |
| Track 4 Record | D18 | Record/Stop button for track 4 |
| Mode Switch | D19 | Mode selection for selected track |
| Clear All | D20 | Clear all tracks button |
| Track 1 Volume | A0 | Volume control potentiometer |
| Track 2 Volume | A1 | Volume control potentiometer |
| Track 3 Volume | A2 | Volume control potentiometer |
| Track 4 Volume | A3 | Volume control potentiometer |
| Master Volume | A4 | Master volume potentiometer |
| Built-in LED | - | Status indicator |

#### Standard Version (FourTrackLooper.cpp)
| Function | Pin | Description |
|----------|-----|-------------|
| Track 1 Record | D15 | Record/Stop button for track 1 |
| Track 2 Record | D16 | Record/Stop button for track 2 |
| Track 3 Record | D17 | Record/Stop button for track 3 |
| Track 4 Record | D18 | Record/Stop button for track 4 |
| Track 1 Mode | D19 | Mode selection button for track 1 |
| Track 2 Mode | D20 | Mode selection button for track 2 |
| Track 3 Mode | D21 | Mode selection button for track 3 |
| Track 4 Mode | D22 | Mode selection button for track 4 |
| Clear All | D23 | Clear all tracks button |
| Master Record | D24 | Master record button (all tracks) |
| Track 1 Volume | A0 | Volume control potentiometer |
| Track 2 Volume | A1 | Volume control potentiometer |
| Track 3 Volume | A2 | Volume control potentiometer |
| Track 4 Volume | A3 | Volume control potentiometer |
| Master Volume | A4 | Master volume potentiometer |
| Built-in LED | - | Status indicator |

#### Advanced Version (FourTrackLooperAdvanced.cpp)
Includes all standard pins plus additional controls for mute, solo, reverse, half-speed, pitch, filter, feedback, pan, tempo, performance mode, and sync. Requires 22+ analog and digital pins. See source code for complete pin mapping.

### Button Wiring
Connect each button between the specified pin and ground. The internal pull-up resistors are used, so no external resistors are needed.

## Building and Flashing

### Prerequisites
- Daisy toolchain installed
- libDaisy and DaisySP libraries
- ARM GCC compiler
- DFU utilities for flashing

### Build Commands

Choose which version to build by editing the Makefile to specify the desired source file:

#### For Simple Version:
```bash
cd DaisyExamples/seed/FourTrackLooper
# Edit Makefile: CPP_SOURCES = FourTrackLooperSimple.cpp
make clean
make
make program-dfu
```

#### For Standard Version:
```bash
cd DaisyExamples/seed/FourTrackLooper
# Edit Makefile: CPP_SOURCES = FourTrackLooper.cpp
make clean
make
make program-dfu
```

#### For Advanced Version:
```bash
cd DaisyExamples/seed/FourTrackLooper
# Edit Makefile: CPP_SOURCES = FourTrackLooperAdvanced.cpp
make clean
make
make program-dfu
```

## Operation Guide

### Recording Your First Loop

1. **Connect your audio source** to the Daisy Seed's audio input
2. **Press Track 1 Record button** to start recording
3. **Press Track 1 Record button again** to stop recording and start looping
4. The track will now loop continuously

### Recording Additional Tracks

1. While Track 1 is looping, **press Track 2 Record button**
2. Record your overdub/harmony part
3. **Press Track 2 Record button again** to start looping Track 2
4. Repeat for Tracks 3 and 4

### Recording Modes

Each track can operate in different modes. Press the **Mode button** for each track to cycle through:

#### Normal Mode (Default for Track 1)
- Input is continuously added to the existing loop while recording
- Perfect for building up layers

#### One-time Dub Mode (Default for Track 2)
- Records exactly one pass through the loop length
- Automatically stops recording after one complete loop
- Great for precise overdubs

#### Replace Mode (Default for Track 3)
- Replaces existing audio in the buffer while recording
- Useful for correcting mistakes or changing parts

#### Frippertronics Mode (Default for Track 4)
- Infinite looping with decay on each loop iteration
- Creates tape delay-like effects
- Named after Robert Fripp's tape loop techniques

### Master Controls

#### Master Record Button
- Arms all tracks for simultaneous recording
- Useful for creating synchronized multi-track recordings

#### Clear All Button
- Immediately clears all tracks
- Use to start fresh or in case of mistakes

### LED Status Indicators

| LED Pattern | Meaning |
|-------------|---------|
| Fast Blink (10Hz) | Recording in progress |
| Slow Blink (1Hz) | Tracks contain content |
| Solid On | Master record armed |
| Off | No activity |

## Tips for Use

### Getting Started
1. Start with Track 1 in Normal mode for your base rhythm or chord progression
2. Use Track 2 in One-time Dub mode for precise melodic overdubs
3. Experiment with Track 3 in Replace mode for dynamic changes
4. Try Track 4 in Frippertronics mode for ambient textures

### Performance Techniques
- **Layered Recording**: Build complexity by recording multiple passes on Normal mode tracks
- **Synchronized Recording**: Use Master Record for tight timing across multiple tracks
- **Dynamic Arrangement**: Use Replace mode to create song sections and changes
- **Ambient Textures**: Frippertronics mode creates evolving soundscapes

### Loop Timing
- The first loop you record sets the timing for all subsequent tracks
- Keep your initial loop tight and rhythmically solid
- All other tracks will automatically sync to this master timing

## Technical Specifications

- **Sample Rate**: 48 kHz
- **Buffer Size**: 4 samples (low latency)
- **Maximum Loop Length**: 30 seconds (Standard/Advanced), 20 seconds (Simple)
- **Total Memory Usage**: ~5.76 MB (Standard/Advanced), ~3.84 MB (Simple)
- **Audio Processing**: Mono input, stereo output
- **Bit Depth**: 32-bit floating point

## Inspiration

This looper is inspired by the Critter & Guitari Five Moons, a beloved hardware looper known for its intuitive workflow and creative possibilities. The Five Moons concept of multiple independent loopers with different behaviors is implemented here with the four distinct recording modes.

## Version Selection Guide

### Choose Simple Version If:
- You want to get started quickly
- You have limited breadboard space
- You prefer straightforward operation
- You don't need individual track mode controls

### Choose Standard Version If:
- You want individual control over each track's mode
- You like having dedicated buttons for common functions
- You want master record functionality
- You have adequate pins and breadboard space

### Choose Advanced Version If:
- You want maximum creative control
- You plan to build a custom PCB
- You want performance-oriented features
- You're comfortable with complex wiring

## Troubleshooting

### No Audio Output
- Check audio connections
- Verify input signal level
- Make sure at least one track has recorded content

### Recording Not Working
- Check button connections
- Verify pin assignments match your wiring
- Make sure you're pressing record twice (start and stop)

### Timing Issues
- Ensure first loop is recorded cleanly
- Subsequent tracks automatically sync to the first loop's timing
- Use Master Record for synchronized multi-track recording

## Future Enhancements

The Advanced version already includes many of these features. Additional possibilities:
- MIDI synchronization and control
- External storage for longer loops (SD card)
- CV/Gate inputs for modular synthesis integration
- USB connectivity for DAW integration
- Multiple output channels for individual track processing
- Real-time loop manipulation via expression pedals

## License

This project uses the Electrosmith DaisySP library and follows the MIT license.