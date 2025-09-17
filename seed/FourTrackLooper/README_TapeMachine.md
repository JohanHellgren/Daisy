# Four-Track Tape Machine for Daisy Seed

A 4-track tape machine inspired by classic multitrack recorders like the Tascam Portastudio series, with modern looper capabilities. Each track can operate as either a **tape loop** (like a traditional looper) or a **linear tape track** (like a cassette recorder), giving you the best of both worlds.

## Concept

This implementation recreates the workflow of classic 4-track tape machines while adding modern looper functionality:

- **Loop Mode**: Traditional looper behavior with continuous overdubbing (30 seconds max)
- **Tape Mode**: Linear recording like a cassette recorder (60 seconds max) 
- **Transport Controls**: Classic tape machine controls (Record, Play, Stop, Rewind)
- **Track Management**: Individual track controls with mode switching
- **Analog Feel**: Volume controls and analog-style monitoring

## Key Features

### Dual Track Modes
- **Loop Tracks**: Perfect loops with seamless overdubbing, ideal for rhythmic patterns and chord progressions
- **Tape Tracks**: Linear recording with start/stop capability, perfect for verses, solos, and arrangements

### Tape Machine Transport
- **Record**: Start recording on selected track (press again to stop and switch to play)
- **Play**: Start playback of recorded content
- **Stop**: Stop all transport (global)
- **Rewind**: Return tape tracks to beginning (global)
- **Individual Control**: Each track has independent record/play buttons

### Memory Management
- **Loop Tracks**: Use internal RAM for highest quality (32-bit float)
- **Tape Tracks**: Use QSPI external memory with 16-bit storage for longer recordings
- **Smart Allocation**: Efficient memory usage allowing 4 full-length tracks

## Hardware Requirements

### Daisy Seed
- Daisy Seed development board
- Audio input/output connections
- Breadboard or custom PCB

### Pin Connections

| Function | Pin | Description |
|----------|-----|-------------|
| **Track Controls** |
| Track 1 Record | D15 | Record/Stop for track 1 |
| Track 2 Record | D16 | Record/Stop for track 2 |
| Track 3 Record | D17 | Record/Stop for track 3 |
| Track 4 Record | D18 | Record/Stop for track 4 |
| Track 1 Play | D19 | Play/Stop for track 1 |
| Track 2 Play | D20 | Play/Stop for track 2 |
| Track 3 Play | D21 | Play/Stop for track 3 |
| Track 4 Play | D22 | Play/Stop for track 4 |
| Track 1 Mode | D23 | Toggle Loop/Tape mode for track 1 |
| Track 2 Mode | D24 | Toggle Loop/Tape mode for track 2 |
| Track 3 Mode | D25 | Toggle Loop/Tape mode for track 3 |
| Track 4 Mode | D26 | Toggle Loop/Tape mode for track 4 |
| **Transport Controls** |
| Stop All | D27 | Stop all tracks |
| Rewind All | D28 | Rewind all tape tracks to beginning |
| Clear All | D29 | Erase all tracks |
| **Analog Controls** |
| Track 1 Volume | A0 | Volume fader for track 1 |
| Track 2 Volume | A1 | Volume fader for track 2 |
| Track 3 Volume | A2 | Volume fader for track 3 |
| Track 4 Volume | A3 | Volume fader for track 4 |
| Master Volume | A4 | Master output level |
| Built-in LED | - | Status indicator |

### Wiring Notes
- All buttons connect between the pin and ground (internal pull-ups used)
- Potentiometers connect: left leg to ground, right leg to 3.3V, wiper to analog pin
- Use logarithmic taper potentiometers for volume controls if available

## Building and Flashing

### Prerequisites
- Daisy toolchain installed
- libDaisy and DaisySP libraries
- ARM GCC compiler
- DFU utilities

### Build Commands
```bash
cd DaisyExamples/seed/FourTrackLooper
# Edit Makefile: CPP_SOURCES = FourTrackTapeMachine.cpp
make clean
make
make program-dfu
```

## Operation Guide

### Basic Workflow

1. **Choose Track Mode**
   - Press a **Mode button** to toggle between Loop and Tape mode for that track
   - Loop mode: For rhythmic loops, bass lines, chord progressions
   - Tape mode: For linear parts like verses, solos, sound effects

2. **Recording Your First Track**
   - Press a **Record button** to start recording on that track
   - Play your part
   - Press the **Record button** again to stop recording and start playback

3. **Overdubbing (Loop Mode Only)**
   - While a loop track is playing, press its **Record button** again
   - This starts overdub mode - new material is mixed with existing content
   - Press **Record** again to stop overdubbing

4. **Linear Recording (Tape Mode)**
   - Tape tracks record linearly like a cassette deck
   - Press **Record** to start, **Record** again to stop
   - Press **Play** to play back from the current position
   - Use **Rewind** to return to the beginning

### Advanced Techniques

#### Creating Song Structures with Tape Tracks
1. Record intro on Tape Track 1
2. Record verse on Tape Track 2  
3. Record chorus on Tape Track 3
4. Use Loop Track 4 for continuous rhythm throughout

#### Layered Loop Building
1. Set Track 1 to Loop mode, record basic rhythm
2. Set Track 2 to Loop mode, overdub bass line
3. Set Track 3 to Loop mode, add chord progression
4. Set Track 4 to Tape mode, record lead melody or solo

#### Performance Mode
- Use **Play** buttons to bring tracks in and out during performance
- Use **Stop All** for dramatic breaks
- Use individual **Record** buttons for live looping

### Transport Controls

#### Global Controls
- **Stop All**: Stops all track playback and recording immediately
- **Rewind All**: Returns all tape tracks to the beginning (loops unaffected)
- **Clear All**: Erases all recorded content on all tracks

#### Per-Track Controls
- **Record**: Start/stop recording on individual tracks
- **Play**: Start/stop playback on individual tracks  
- **Mode**: Toggle between Loop and Tape modes

### LED Status Indicators

| LED Pattern | Meaning |
|-------------|---------|
| Fast Blink (5Hz) | Recording in progress |
| Slow Blink (1Hz) | Playback active |
| Solid Off | All tracks stopped |

## Track Mode Details

### Loop Mode
- **Duration**: Up to 30 seconds
- **Quality**: Full 32-bit float resolution
- **Behavior**: Seamless looping with overdub capability
- **Memory**: Internal RAM
- **Best For**: Rhythms, chord progressions, bass lines, percussion

### Tape Mode  
- **Duration**: Up to 60 seconds
- **Quality**: 16-bit resolution (still excellent quality)
- **Behavior**: Linear recording with start/stop transport
- **Memory**: External QSPI flash
- **Best For**: Verses, solos, sound effects, arrangements

## Technical Specifications

- **Sample Rate**: 48 kHz
- **Loop Resolution**: 32-bit float (internal RAM)
- **Tape Resolution**: 16-bit (external QSPI memory)
- **Maximum Loop Length**: 30 seconds per track
- **Maximum Tape Length**: 60 seconds per track
- **Total Memory Usage**: 
  - Internal RAM: ~5.76 MB for loop buffers
  - QSPI Memory: ~23 MB for tape buffers
- **Latency**: ~0.08ms (4 samples at 48kHz)

## Workflow Examples

### Singer-Songwriter Setup
1. **Track 1 (Loop)**: Fingerpicked guitar pattern
2. **Track 2 (Loop)**: Vocal harmony loop  
3. **Track 3 (Tape)**: Lead vocal verse
4. **Track 4 (Tape)**: Lead vocal chorus

### Electronic Music Setup
1. **Track 1 (Loop)**: Drum machine pattern
2. **Track 2 (Loop)**: Bass synthesizer sequence
3. **Track 3 (Loop)**: Chord pad progression
4. **Track 4 (Tape)**: Lead synthesizer melody

### Experimental/Ambient Setup
1. **Track 1 (Tape)**: Field recording or texture
2. **Track 2 (Loop)**: Minimal melodic pattern
3. **Track 3 (Loop)**: Drone or pad sound
4. **Track 4 (Tape)**: Processed instrument or voice

## Tips and Tricks

### Recording Techniques
- **Start with Rhythm**: Use Loop Track 1 for your main rhythmic element
- **Build in Layers**: Add harmonic and melodic elements on subsequent tracks
- **Use Tape for Dynamics**: Record dynamics and arrangement changes on tape tracks
- **Monitor Levels**: Adjust individual track volumes for proper balance

### Performance Tips
- **Practice Transitions**: Get comfortable with the transport controls
- **Use Global Stop**: Create dramatic breaks in your performance
- **Layer Strategically**: Don't fill all tracks immediately - leave room to grow
- **Mix Live**: Use volume controls to create live mix automation

### Creative Possibilities
- **Backwards Compatibility**: Record something, rewind, then overdub for layered effects
- **Track Bouncing**: Record the output of multiple tracks onto a tape track
- **Live Sampling**: Use tape mode to capture live environmental sounds
- **Loop Discovery**: Use tape mode to find interesting sections, then recreate as loops

## Troubleshooting

### No Audio Output
- Check audio connections and cable integrity
- Verify master volume is up
- Ensure at least one track has recorded content and is playing

### Recording Not Working  
- Check button connections and wiring
- Verify input signal is present
- Make sure track is not muted (volume knob up)

### Memory Issues
- If experiencing crashes, reduce TAPE_MAX_TIME in the source code
- QSPI memory issues may require hardware inspection

### Timing Problems
- Ensure first recorded element provides solid timing reference
- Use metronome or click track for initial recording if needed

## Inspiration and History

This design draws inspiration from:
- **Tascam Portastudio**: The original 4-track cassette recorder that revolutionized home recording
- **Loop Stations**: Modern looper pedals and devices
- **Multitrack Workflow**: Classic studio techniques adapted for live performance
- **Tape Culture**: The aesthetic and workflow of analog tape recording

The combination of loop and tape modes gives you the immediate creative feedback of loopers with the arrangement capabilities of multitrack recording.

## License

This project uses the Electrosmith DaisySP library and follows the MIT license.