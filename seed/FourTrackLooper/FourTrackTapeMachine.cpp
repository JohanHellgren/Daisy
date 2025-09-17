#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

// Constants
#define NUM_TRACKS 4
#define SAMPLE_RATE 48000
#define LOOP_MAX_TIME 30.0f     // 30 seconds for loop tracks
#define TAPE_MAX_TIME 60.0f     // 60 seconds for tape tracks
#define LOOP_BUFFER_SIZE static_cast<size_t>(SAMPLE_RATE * LOOP_MAX_TIME)
#define TAPE_BUFFER_SIZE static_cast<size_t>(SAMPLE_RATE * TAPE_MAX_TIME)

// Hardware
static DaisySeed hw;

// Track modes
enum TrackMode
{
    MODE_LOOP = 0,
    MODE_TAPE = 1
};

// Track states
enum TrackState
{
    STATE_STOPPED = 0,
    STATE_PLAYING = 1,
    STATE_RECORDING = 2,
    STATE_OVERDUBBING = 3
};

// Track class for tape machine functionality
class TapeMachineTrack
{
  public:
    // Mode and state
    TrackMode mode;
    TrackState state;
    
    // Audio buffers
    float loop_buffer[LOOP_BUFFER_SIZE];      // Internal RAM for loops
    int16_t* tape_buffer;                     // QSPI memory for tape (16-bit)
    
    // Transport
    float playhead_position;
    float record_position;
    float track_length;
    bool armed;
    bool muted;
    float volume;
    
    // Loop-specific
    Looper looper;
    bool loop_has_content;
    
    void Init(int16_t* qspi_buffer_start)
    {
        mode = MODE_LOOP;
        state = STATE_STOPPED;
        
        // Initialize buffers
        tape_buffer = qspi_buffer_start;
        looper.Init(loop_buffer, LOOP_BUFFER_SIZE);
        
        // Reset transport
        playhead_position = 0.0f;
        record_position = 0.0f;
        track_length = 0.0f;
        armed = false;
        muted = false;
        volume = 0.8f;
        loop_has_content = false;
        
        // Clear tape buffer
        for(size_t i = 0; i < TAPE_BUFFER_SIZE; i++)
        {
            tape_buffer[i] = 0;
        }
    }
    
    float Process(float input)
    {
        float output = 0.0f;
        
        if(mode == MODE_LOOP)
        {
            // Loop mode processing
            switch(state)
            {
                case STATE_STOPPED:
                    output = 0.0f;
                    break;
                    
                case STATE_RECORDING:
                    output = looper.Process(input);
                    if(!looper.Recording())
                    {
                        state = STATE_PLAYING;
                        loop_has_content = true;
                    }
                    break;
                    
                case STATE_PLAYING:
                    output = looper.Process(0.0f);
                    break;
                    
                case STATE_OVERDUBBING:
                    output = looper.Process(input);
                    break;
            }
        }
        else // MODE_TAPE
        {
            // Tape mode processing
            switch(state)
            {
                case STATE_STOPPED:
                    output = 0.0f;
                    break;
                    
                case STATE_RECORDING:
                    if(record_position < TAPE_BUFFER_SIZE - 1)
                    {
                        // Record to tape buffer (convert float to 16-bit)
                        tape_buffer[static_cast<size_t>(record_position)] = 
                            static_cast<int16_t>(input * 32767.0f);
                        record_position += 1.0f;
                        track_length = record_position;
                    }
                    else
                    {
                        // End of tape reached
                        state = STATE_STOPPED;
                    }
                    output = input * 0.5f; // Monitor while recording
                    break;
                    
                case STATE_PLAYING:
                    if(playhead_position < track_length)
                    {
                        // Playback from tape buffer (convert 16-bit to float)
                        size_t pos = static_cast<size_t>(playhead_position);
                        output = static_cast<float>(tape_buffer[pos]) / 32767.0f;
                        playhead_position += 1.0f;
                    }
                    else
                    {
                        // End of recorded content
                        output = 0.0f;
                    }
                    break;
                    
                case STATE_OVERDUBBING:
                    if(playhead_position < track_length)
                    {
                        // Mix input with existing content
                        size_t pos = static_cast<size_t>(playhead_position);
                        float existing = static_cast<float>(tape_buffer[pos]) / 32767.0f;
                        float mixed = (existing + input) * 0.7f;
                        tape_buffer[pos] = static_cast<int16_t>(mixed * 32767.0f);
                        output = mixed;
                        playhead_position += 1.0f;
                    }
                    else
                    {
                        state = STATE_STOPPED;
                        output = 0.0f;
                    }
                    break;
            }
        }
        
        // Apply volume and mute
        if(muted) return 0.0f;
        return output * volume;
    }
    
    void StartRecording()
    {
        if(mode == MODE_LOOP)
        {
            if(loop_has_content && state == STATE_PLAYING)
            {
                state = STATE_OVERDUBBING;
                looper.TrigRecord();
            }
            else
            {
                state = STATE_RECORDING;
                looper.TrigRecord();
            }
        }
        else // MODE_TAPE
        {
            if(track_length > 0 && state == STATE_PLAYING)
            {
                // Overdub mode - continue from current position
                state = STATE_OVERDUBBING;
            }
            else
            {
                // Fresh recording
                state = STATE_RECORDING;
                record_position = 0.0f;
                track_length = 0.0f;
            }
        }
    }
    
    void StartPlayback()
    {
        if(mode == MODE_LOOP)
        {
            if(loop_has_content)
            {
                state = STATE_PLAYING;
            }
        }
        else // MODE_TAPE
        {
            if(track_length > 0)
            {
                state = STATE_PLAYING;
                if(playhead_position >= track_length)
                {
                    playhead_position = 0.0f; // Restart from beginning
                }
            }
        }
    }
    
    void Stop()
    {
        if(mode == MODE_LOOP)
        {
            if(state == STATE_RECORDING || state == STATE_OVERDUBBING)
            {
                looper.TrigRecord(); // Stop recording, start playback
                state = STATE_PLAYING;
                loop_has_content = true;
            }
            else
            {
                state = STATE_STOPPED;
            }
        }
        else // MODE_TAPE
        {
            state = STATE_STOPPED;
        }
    }
    
    void Rewind()
    {
        if(mode == MODE_TAPE)
        {
            playhead_position = 0.0f;
            if(state == STATE_PLAYING)
            {
                // Keep playing from beginning
            }
        }
        else // MODE_LOOP
        {
            // Loops don't need rewinding, but we can restart
            if(state == STATE_PLAYING)
            {
                looper.Clear();
                looper.TrigRecord();
                state = STATE_PLAYING;
            }
        }
    }
    
    void Clear()
    {
        if(mode == MODE_LOOP)
        {
            looper.Clear();
            loop_has_content = false;
        }
        else // MODE_TAPE
        {
            for(size_t i = 0; i < TAPE_BUFFER_SIZE; i++)
            {
                tape_buffer[i] = 0;
            }
            track_length = 0.0f;
        }
        state = STATE_STOPPED;
        playhead_position = 0.0f;
        record_position = 0.0f;
    }
    
    void SetMode(TrackMode new_mode)
    {
        if(new_mode != mode)
        {
            Stop();
            mode = new_mode;
        }
    }
    
    void ToggleMode()
    {
        SetMode(mode == MODE_LOOP ? MODE_TAPE : MODE_LOOP);
    }
    
    void SetVolume(float vol) { volume = fclamp(vol, 0.0f, 1.0f); }
    void ToggleMute() { muted = !muted; }
    bool IsRecording() { return (state == STATE_RECORDING || state == STATE_OVERDUBBING); }
    bool IsPlaying() { return (state == STATE_PLAYING || state == STATE_OVERDUBBING); }
    bool HasContent() 
    { 
        return mode == MODE_LOOP ? loop_has_content : (track_length > 0);
    }
    
    float GetPosition()
    {
        if(mode == MODE_LOOP)
        {
            return 0.0f; // Loops don't have linear position
        }
        else
        {
            return track_length > 0 ? (playhead_position / track_length) : 0.0f;
        }
    }
};

// Global objects
static TapeMachineTrack tracks[NUM_TRACKS];

// QSPI buffer allocation
static int16_t DSY_QSPI_BSS qspi_tape_buffer[NUM_TRACKS * TAPE_BUFFER_SIZE];

// Controls
static Switch record_buttons[NUM_TRACKS];
static Switch play_buttons[NUM_TRACKS];
static Switch mode_buttons[NUM_TRACKS];
static Switch stop_button;
static Switch rewind_button;
static Switch clear_all_button;

static AnalogControl volume_controls[NUM_TRACKS];
static AnalogControl master_volume;

// State
static int selected_track = 0;
static float master_vol = 1.0f;
static bool global_sync = false;

// LED feedback
static uint32_t led_timer = 0;
static bool led_state = false;

void InitControls()
{
    // Track controls
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        record_buttons[i].Init(hw.GetPin(15 + i), 1000.0f);
        play_buttons[i].Init(hw.GetPin(19 + i), 1000.0f);
        mode_buttons[i].Init(hw.GetPin(23 + i), 1000.0f);
    }
    
    // Transport controls
    stop_button.Init(hw.GetPin(27), 1000.0f);
    rewind_button.Init(hw.GetPin(28), 1000.0f);
    clear_all_button.Init(hw.GetPin(29), 1000.0f);
    
    // Analog controls
    AdcChannelConfig adc_config[5];
    for(int i = 0; i < 5; i++)
    {
        adc_config[i].InitSingle(hw.GetPin(A0 + i));
    }
    hw.adc.Init(adc_config, 5);
    
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        volume_controls[i].Init(hw.adc.GetPtr(i), hw.AudioCallbackRate());
    }
    master_volume.Init(hw.adc.GetPtr(4), hw.AudioCallbackRate());
}

void UpdateControls()
{
    // Update volume controls
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        tracks[i].SetVolume(volume_controls[i].Process());
    }
    master_vol = master_volume.Process();
    
    // Update buttons
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        record_buttons[i].Debounce();
        play_buttons[i].Debounce();
        mode_buttons[i].Debounce();
        
        // Record button
        if(record_buttons[i].RisingEdge())
        {
            selected_track = i;
            if(tracks[i].IsRecording())
            {
                tracks[i].Stop();
            }
            else
            {
                tracks[i].StartRecording();
            }
        }
        
        // Play button
        if(play_buttons[i].RisingEdge())
        {
            selected_track = i;
            if(tracks[i].IsPlaying() && !tracks[i].IsRecording())
            {
                tracks[i].Stop();
            }
            else
            {
                tracks[i].StartPlayback();
            }
        }
        
        // Mode button
        if(mode_buttons[i].RisingEdge())
        {
            tracks[i].ToggleMode();
        }
    }
    
    // Transport controls
    stop_button.Debounce();
    rewind_button.Debounce();
    clear_all_button.Debounce();
    
    if(stop_button.RisingEdge())
    {
        // Stop all tracks
        for(int i = 0; i < NUM_TRACKS; i++)
        {
            tracks[i].Stop();
        }
    }
    
    if(rewind_button.RisingEdge())
    {
        // Rewind all tracks
        for(int i = 0; i < NUM_TRACKS; i++)
        {
            tracks[i].Rewind();
        }
    }
    
    if(clear_all_button.RisingEdge())
    {
        // Clear all tracks
        for(int i = 0; i < NUM_TRACKS; i++)
        {
            tracks[i].Clear();
        }
    }
}

void UpdateLED()
{
    uint32_t now = System::GetNow();
    int recording_tracks = 0;
    int playing_tracks = 0;
    
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        if(tracks[i].IsRecording()) recording_tracks++;
        if(tracks[i].IsPlaying()) playing_tracks++;
    }
    
    if(recording_tracks > 0)
    {
        // Fast blink when recording
        if(now - led_timer > 100)
        {
            led_state = !led_state;
            led_timer = now;
        }
    }
    else if(playing_tracks > 0)
    {
        // Slow blink when playing
        if(now - led_timer > 500)
        {
            led_state = !led_state;
            led_timer = now;
        }
    }
    else
    {
        led_state = false;
    }
    
    hw.SetLed(led_state);
}

static void AudioCallback(AudioHandle::InputBuffer  in,
                          AudioHandle::OutputBuffer out,
                          size_t                    size)
{
    for(size_t i = 0; i < size; i++)
    {
        float input_left = in[0][i];
        float input_right = in[1][i];
        float mono_input = (input_left + input_right) * 0.5f;
        
        float mixed_output = 0.0f;
        int active_tracks = 0;
        
        // Process all tracks
        for(int t = 0; t < NUM_TRACKS; t++)
        {
            float track_output = tracks[t].Process(mono_input);
            mixed_output += track_output;
            if(fabs(track_output) > 0.001f)
            {
                active_tracks++;
            }
        }
        
        // Apply master volume
        mixed_output *= master_vol;
        
        // Soft limiting
        mixed_output = tanhf(mixed_output);
        
        out[0][i] = mixed_output;
        out[1][i] = mixed_output;
    }
}

int main(void)
{
    // Initialize hardware
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    
    // Initialize tracks with QSPI buffer allocation
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        tracks[i].Init(&qspi_tape_buffer[i * TAPE_BUFFER_SIZE]);
    }
    
    // Initialize controls
    InitControls();
    hw.adc.Start();
    
    // Start audio
    hw.StartAudio(AudioCallback);
    
    // Main loop
    while(1)
    {
        UpdateControls();
        UpdateLED();
        System::Delay(2);
    }
}