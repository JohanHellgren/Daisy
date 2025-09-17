#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

// Constants
#define NUM_TRACKS 4
#define SAMPLE_RATE 48000
#define MAX_LOOP_TIME 20.0f  // 20 seconds max per track
#define BUFFER_SIZE static_cast<size_t>(SAMPLE_RATE * MAX_LOOP_TIME)

// Hardware
static DaisySeed hw;

// Simple Track class for looper functionality
class SimpleLooperTrack
{
  public:
    Looper looper;
    float volume;
    bool muted;
    float buffer[BUFFER_SIZE];
    
    void Init()
    {
        looper.Init(buffer, BUFFER_SIZE);
        volume = 0.8f;
        muted = false;
    }
    
    float Process(float input)
    {
        float output = looper.Process(input);
        if (muted) return 0.0f;
        return output * volume;
    }
    
    void TriggerRecord() { looper.TrigRecord(); }
    void Clear() { looper.Clear(); }
    bool IsRecording() { return looper.Recording(); }
    void SetVolume(float vol) { volume = fclamp(vol, 0.0f, 1.0f); }
    void ToggleMute() { muted = !muted; }
    void SetMode(Looper::Mode mode) { looper.SetMode(mode); }
    Looper::Mode GetMode() { return looper.GetMode(); }
    void IncrementMode() { looper.IncrementMode(); }
};

// Global objects
static SimpleLooperTrack tracks[NUM_TRACKS];

// Simple control interface - minimal pins needed
static Switch record_buttons[NUM_TRACKS];
static Switch mode_switch;
static Switch clear_all_switch;
static AnalogControl volume_knobs[NUM_TRACKS];
static AnalogControl master_volume;

// State variables
static int selected_track = 0;
static float master_vol = 1.0f;
static uint32_t button_timer = 0;

// LED feedback
static uint32_t led_timer = 0;
static bool led_state = false;
static int led_blink_pattern = 0;

void InitControls()
{
    // Record buttons for each track (pins D15, D16, D17, D18)
    record_buttons[0].Init(hw.GetPin(15), 1000.0f);
    record_buttons[1].Init(hw.GetPin(16), 1000.0f);
    record_buttons[2].Init(hw.GetPin(17), 1000.0f);
    record_buttons[3].Init(hw.GetPin(18), 1000.0f);
    
    // Mode switch (pin D19) - cycles through track modes
    mode_switch.Init(hw.GetPin(19), 1000.0f);
    
    // Clear all switch (pin D20)
    clear_all_switch.Init(hw.GetPin(20), 1000.0f);
    
    // Initialize ADC for volume controls
    AdcChannelConfig adc_config[5];
    adc_config[0].InitSingle(hw.GetPin(A0)); // Track 1 volume
    adc_config[1].InitSingle(hw.GetPin(A1)); // Track 2 volume
    adc_config[2].InitSingle(hw.GetPin(A2)); // Track 3 volume
    adc_config[3].InitSingle(hw.GetPin(A3)); // Track 4 volume
    adc_config[4].InitSingle(hw.GetPin(A4)); // Master volume
    
    hw.adc.Init(adc_config, 5);
    
    // Initialize volume controls
    volume_knobs[0].Init(hw.adc.GetPtr(0), hw.AudioCallbackRate());
    volume_knobs[1].Init(hw.adc.GetPtr(1), hw.AudioCallbackRate());
    volume_knobs[2].Init(hw.adc.GetPtr(2), hw.AudioCallbackRate());
    volume_knobs[3].Init(hw.adc.GetPtr(3), hw.AudioCallbackRate());
    master_volume.Init(hw.adc.GetPtr(4), hw.AudioCallbackRate());
}

void UpdateControls()
{
    uint32_t now = System::GetNow();
    
    // Update volume controls
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        tracks[i].SetVolume(volume_knobs[i].Process());
    }
    master_vol = master_volume.Process();
    
    // Update button states
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        record_buttons[i].Debounce();
        
        // Handle record button presses
        if(record_buttons[i].RisingEdge())
        {
            tracks[i].TriggerRecord();
            selected_track = i; // Track selection follows last pressed record button
        }
        
        // Double-tap to clear individual track
        if(record_buttons[i].FallingEdge())
        {
            button_timer = now;
        }
        if(record_buttons[i].RisingEdge() && (now - button_timer) < 300)
        {
            tracks[i].Clear();
        }
    }
    
    mode_switch.Debounce();
    clear_all_switch.Debounce();
    
    // Handle mode switching for selected track
    if(mode_switch.RisingEdge())
    {
        tracks[selected_track].IncrementMode();
    }
    
    // Handle clear all
    if(clear_all_switch.RisingEdge())
    {
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
    int active_tracks = 0;
    
    // Count track states
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        if(tracks[i].IsRecording())
        {
            recording_tracks++;
        }
        else if(tracks[i].volume > 0.01f && !tracks[i].muted)
        {
            active_tracks++;
        }
    }
    
    // LED patterns based on state
    if(recording_tracks > 0)
    {
        // Fast blink when recording (rate depends on number of recording tracks)
        uint32_t blink_rate = 100 / recording_tracks; // Faster with more tracks
        if(now - led_timer > blink_rate)
        {
            led_state = !led_state;
            led_timer = now;
        }
    }
    else if(active_tracks > 0)
    {
        // Different patterns for different numbers of active tracks
        switch(active_tracks)
        {
            case 1:
                // Slow blink for 1 track
                if(now - led_timer > 800)
                {
                    led_state = !led_state;
                    led_timer = now;
                }
                break;
            case 2:
                // Double blink for 2 tracks
                led_blink_pattern = (now / 200) % 6;
                led_state = (led_blink_pattern == 0 || led_blink_pattern == 1 || led_blink_pattern == 3 || led_blink_pattern == 4);
                break;
            case 3:
                // Triple blink for 3 tracks
                led_blink_pattern = (now / 150) % 8;
                led_state = (led_blink_pattern < 6 && (led_blink_pattern % 2) == 0);
                break;
            case 4:
                // Rapid pulse for all 4 tracks
                if(now - led_timer > 300)
                {
                    led_state = !led_state;
                    led_timer = now;
                }
                break;
        }
    }
    else
    {
        // Off when no activity
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
        
        // Mix input to mono for processing
        float mono_input = (input_left + input_right) * 0.5f;
        
        // Process all tracks and mix
        float mixed_output = 0.0f;
        int active_count = 0;
        
        for(int t = 0; t < NUM_TRACKS; t++)
        {
            float track_output = tracks[t].Process(mono_input);
            if(fabs(track_output) > 0.001f)
            {
                mixed_output += track_output;
                active_count++;
            }
        }
        
        // Automatic gain control for multiple tracks
        if(active_count > 1)
        {
            mixed_output *= (1.0f / sqrtf(active_count));
        }
        
        // Apply master volume
        mixed_output *= master_vol;
        
        // Add minimal dry signal for monitoring while recording
        bool any_recording = false;
        for(int t = 0; t < NUM_TRACKS; t++)
        {
            if(tracks[t].IsRecording())
            {
                any_recording = true;
                break;
            }
        }
        
        if(any_recording)
        {
            mixed_output += mono_input * 0.1f; // 10% dry signal while recording
        }
        
        // Soft limiting to prevent clipping
        mixed_output = tanhf(mixed_output * 0.9f);
        
        // Output to both channels
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
    
    // Initialize tracks with different modes for creative variety
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        tracks[i].Init();
    }
    
    // Set default modes that work well together
    tracks[0].SetMode(Looper::Mode::NORMAL);          // Base layer - continuous overdub
    tracks[1].SetMode(Looper::Mode::ONETIME_DUB);     // Precise overdubs
    tracks[2].SetMode(Looper::Mode::REPLACE);         // Dynamic changes
    tracks[3].SetMode(Looper::Mode::FRIPPERTRONICS);  // Ambient textures
    
    // Initialize controls
    InitControls();
    
    // Start ADC
    hw.adc.Start();
    
    // Start audio processing
    hw.StartAudio(AudioCallback);
    
    // Main control loop
    while(1)
    {
        UpdateControls();
        UpdateLED();
        System::Delay(2); // Small delay for stability
    }
}