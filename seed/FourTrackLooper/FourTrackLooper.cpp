#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

// Constants
#define NUM_TRACKS 4
#define SAMPLE_RATE 48000
#define MAX_LOOP_TIME 30.0f  // 30 seconds max per track
#define BUFFER_SIZE static_cast<size_t>(SAMPLE_RATE * MAX_LOOP_TIME)

// Hardware
static DaisySeed hw;
static AnalogControl volume_controls[NUM_TRACKS];
static AnalogControl master_volume_control;

// Track class to manage individual looper functionality
class LooperTrack
{
  public:
    Looper looper;
    float  volume;
    bool   muted;
    bool   solo;
    float  buffer[BUFFER_SIZE];
    
    void Init()
    {
        looper.Init(buffer, BUFFER_SIZE);
        volume = 1.0f;
        muted = false;
        solo = false;
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
    bool IsRecordingQueued() { return looper.RecordingQueued(); }
    void SetVolume(float vol) { volume = fclamp(vol, 0.0f, 1.0f); }
    void ToggleMute() { muted = !muted; }
    void ToggleSolo() { solo = !solo; }
    void SetMode(Looper::Mode mode) { looper.SetMode(mode); }
    void ToggleReverse() { looper.ToggleReverse(); }
    void ToggleHalfSpeed() { looper.ToggleHalfSpeed(); }
};

// Global objects
static LooperTrack tracks[NUM_TRACKS];
static Switch record_buttons[NUM_TRACKS];
static Switch mode_buttons[NUM_TRACKS];
static Switch clear_button;
static Switch master_record;

// Control variables
static bool master_recording = false;
static float master_volume = 1.0f;
static bool any_solo_active = false;

// LED blink timing
static uint32_t led_timer = 0;
static bool led_state = false;

void InitControls()
{
    // Initialize record buttons for each track (pins 15, 16, 17, 18)
    record_buttons[0].Init(hw.GetPin(15), 1000.0f);
    record_buttons[1].Init(hw.GetPin(16), 1000.0f);
    record_buttons[2].Init(hw.GetPin(17), 1000.0f);
    record_buttons[3].Init(hw.GetPin(18), 1000.0f);
    
    // Initialize mode buttons for each track (pins 19, 20, 21, 22)
    mode_buttons[0].Init(hw.GetPin(19), 1000.0f);
    mode_buttons[1].Init(hw.GetPin(20), 1000.0f);
    mode_buttons[2].Init(hw.GetPin(21), 1000.0f);
    mode_buttons[3].Init(hw.GetPin(22), 1000.0f);
    
    // Clear all tracks button (pin 23)
    clear_button.Init(hw.GetPin(23), 1000.0f);
    
    // Master record button (pin 24)
    master_record.Init(hw.GetPin(24), 1000.0f);
    
    // Initialize volume controls (ADC pins A0-A3 for tracks, A4 for master)
    AdcChannelConfig adc_config[5];
    adc_config[0].InitSingle(hw.GetPin(A0));
    adc_config[1].InitSingle(hw.GetPin(A1));
    adc_config[2].InitSingle(hw.GetPin(A2));
    adc_config[3].InitSingle(hw.GetPin(A3));
    adc_config[4].InitSingle(hw.GetPin(A4));
    
    hw.adc.Init(adc_config, 5);
    
    volume_controls[0].Init(hw.adc.GetPtr(0), hw.AudioCallbackRate());
    volume_controls[1].Init(hw.adc.GetPtr(1), hw.AudioCallbackRate());
    volume_controls[2].Init(hw.adc.GetPtr(2), hw.AudioCallbackRate());
    volume_controls[3].Init(hw.adc.GetPtr(3), hw.AudioCallbackRate());
    master_volume_control.Init(hw.adc.GetPtr(4), hw.AudioCallbackRate());
}

void UpdateControls()
{
    // Update volume controls
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        tracks[i].SetVolume(volume_controls[i].Process());
    }
    master_volume = master_volume_control.Process();
    
    // Update all button states
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        record_buttons[i].Debounce();
        mode_buttons[i].Debounce();
        
        // Handle record button presses
        if(record_buttons[i].RisingEdge())
        {
            tracks[i].TriggerRecord();
        }
        
        // Handle mode button presses (cycle through looper modes)
        if(mode_buttons[i].RisingEdge())
        {
            tracks[i].looper.IncrementMode();
        }
    }
    
    clear_button.Debounce();
    master_record.Debounce();
    
    // Handle clear all tracks
    if(clear_button.RisingEdge())
    {
        for(int i = 0; i < NUM_TRACKS; i++)
        {
            tracks[i].Clear();
        }
    }
    
    // Handle master record (records to all tracks simultaneously)
    if(master_record.RisingEdge())
    {
        master_recording = !master_recording;
        if(master_recording)
        {
            for(int i = 0; i < NUM_TRACKS; i++)
            {
                tracks[i].TriggerRecord();
            }
        }
    }
}

void UpdateLED()
{
    // Enhanced LED indication logic
    bool any_recording = false;
    bool any_queued = false;
    int recording_count = 0;
    int active_tracks = 0;
    
    // Count recording tracks and active tracks
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        if(tracks[i].IsRecording())
        {
            any_recording = true;
            recording_count++;
        }
        if(tracks[i].IsRecordingQueued())
        {
            any_queued = true;
        }
        // Check if track has content (rough estimation)
        if(tracks[i].volume > 0.01f && !tracks[i].muted)
        {
            active_tracks++;
        }
    }
    
    uint32_t now = System::GetNow();
    
    if(any_recording)
    {
        // Variable blink rate based on number of recording tracks
        uint32_t blink_rate = 50 + (recording_count * 25); // Faster with more tracks
        if(now - led_timer > blink_rate)
        {
            led_state = !led_state;
            led_timer = now;
        }
    }
    else if(any_queued)
    {
        // Double blink pattern when recording is queued
        static int blink_phase = 0;
        if(now - led_timer > 100)
        {
            blink_phase = (blink_phase + 1) % 6;
            led_state = (blink_phase == 0 || blink_phase == 1 || blink_phase == 2 || blink_phase == 3);
            led_timer = now;
        }
    }
    else if(master_recording)
    {
        // Solid on when master record is armed
        led_state = true;
    }
    else if(active_tracks > 0)
    {
        // Different patterns based on number of active tracks
        uint32_t pattern_rate = 1000 / (active_tracks + 1); // Faster with more tracks
        if(now - led_timer > pattern_rate)
        {
            led_state = !led_state;
            led_timer = now;
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
        
        // Mix input channels for mono processing
        float mono_input = (input_left + input_right) * 0.5f;
        
        float mixed_output = 0.0f;
        int active_tracks = 0;
        
        // Check if any tracks are soloed
        any_solo_active = false;
        for(int t = 0; t < NUM_TRACKS; t++)
        {
            if(tracks[t].solo)
            {
                any_solo_active = true;
                break;
            }
        }
        
        // Process each track
        for(int t = 0; t < NUM_TRACKS; t++)
        {
            float track_output = tracks[t].Process(mono_input);
            
            // Apply solo logic
            if(any_solo_active)
            {
                if(tracks[t].solo)
                {
                    mixed_output += track_output;
                    active_tracks++;
                }
            }
            else
            {
                mixed_output += track_output;
                active_tracks++;
            }
        }
        
        // Normalize output if multiple tracks are active
        if(active_tracks > 1)
        {
            mixed_output /= active_tracks;
        }
        
        // Apply master volume
        mixed_output *= master_volume;
        
        // Add dry input signal at lower level for monitoring
        float dry_level = 0.1f;
        mixed_output += mono_input * dry_level;
        
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
    
    // Start ADC conversions
    hw.adc.Start();
    
    // Initialize tracks
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        tracks[i].Init();
        // Set different modes for variety
        switch(i)
        {
            case 0: tracks[i].SetMode(Looper::Mode::NORMAL); break;
            case 1: tracks[i].SetMode(Looper::Mode::ONETIME_DUB); break;
            case 2: tracks[i].SetMode(Looper::Mode::REPLACE); break;
            case 3: tracks[i].SetMode(Looper::Mode::FRIPPERTRONICS); break;
        }
    }
    
    // Initialize controls
    InitControls();
    
    // Start audio processing
    hw.StartAudio(AudioCallback);
    
    // Main loop
    while(1)
    {
        UpdateControls();
        UpdateLED();
        System::Delay(1); // Small delay to prevent excessive CPU usage
    }
}