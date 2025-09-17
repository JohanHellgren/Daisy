#include "daisysp.h"
#include "daisy_seed.h"

using namespace daisysp;
using namespace daisy;

// Constants
#define NUM_TRACKS 4
#define SAMPLE_RATE 48000
#define MAX_LOOP_TIME 45.0f  // 45 seconds max per track
#define BUFFER_SIZE static_cast<size_t>(SAMPLE_RATE * MAX_LOOP_TIME)

// Hardware
static DaisySeed hw;

// Advanced Track class with creative controls
class AdvancedLooperTrack
{
  public:
    Looper looper;
    
    // Audio processing
    float volume;
    float pan;
    bool muted;
    bool solo;
    
    // Creative controls
    bool reverse;
    bool half_speed;
    float pitch_shift;
    float filter_cutoff;
    float feedback_amount;
    
    // Effects
    Svf filter;
    PitchShifter pitcher;
    DelayLine<float, 4800> delay; // 0.1 second delay line for effects
    
    // Buffer and state
    float buffer[BUFFER_SIZE];
    bool has_content;
    uint32_t loop_length;
    uint32_t sync_point;
    
    // Performance controls
    bool performance_mode;
    float crossfade_target;
    float crossfade_current;
    
    void Init(float sample_rate)
    {
        looper.Init(buffer, BUFFER_SIZE);
        filter.Init(sample_rate);
        pitcher.Init(sample_rate);
        delay.Init();
        
        volume = 1.0f;
        pan = 0.0f;  // Center
        muted = false;
        solo = false;
        reverse = false;
        half_speed = false;
        pitch_shift = 1.0f;
        filter_cutoff = 1000.0f;
        feedback_amount = 0.0f;
        has_content = false;
        loop_length = 0;
        sync_point = 0;
        performance_mode = false;
        crossfade_target = 1.0f;
        crossfade_current = 1.0f;
        
        // Initialize effects
        filter.SetFreq(filter_cutoff);
        filter.SetRes(0.1f);
        filter.SetDrive(0.1f);
        pitcher.SetTransposition(0.0f);
        delay.SetDelay(sample_rate * 0.05f); // 50ms delay
    }
    
    float Process(float input, bool sync_trigger = false)
    {
        // Handle sync triggers for tight timing
        if(sync_trigger && looper.IsNearBeginning())
        {
            sync_point = 0;
        }
        
        // Process looper
        float output = looper.Process(input);
        
        // Mark as having content if we're playing back
        if(!looper.Recording() && fabs(output) > 0.001f)
        {
            has_content = true;
        }
        
        // Apply creative controls
        if(reverse)
        {
            looper.SetReverse(true);
        }
        
        if(half_speed)
        {
            looper.SetHalfSpeed(true);
        }
        
        // Apply effects
        if(pitch_shift != 1.0f)
        {
            float pitch_semitones = 12.0f * log2f(pitch_shift);
            pitcher.SetTransposition(pitch_semitones);
            output = pitcher.Process(output);
        }
        
        // Apply filtering
        filter.SetFreq(filter_cutoff);
        filter.Process(output);
        output = filter.Low();
        
        // Apply feedback/delay effect
        if(feedback_amount > 0.0f)
        {
            float delayed = delay.Read();
            delay.Write(output + delayed * feedback_amount * 0.3f);
            output += delayed * feedback_amount;
        }
        else
        {
            delay.Write(output);
        }
        
        // Apply volume and crossfading
        if(performance_mode)
        {
            // Smooth crossfade
            float rate = 0.001f;
            if(crossfade_current < crossfade_target)
            {
                crossfade_current += rate;
                if(crossfade_current > crossfade_target)
                    crossfade_current = crossfade_target;
            }
            else if(crossfade_current > crossfade_target)
            {
                crossfade_current -= rate;
                if(crossfade_current < crossfade_target)
                    crossfade_current = crossfade_target;
            }
            output *= volume * crossfade_current;
        }
        else
        {
            output *= volume;
        }
        
        // Apply mute
        if(muted) return 0.0f;
        
        return output;
    }
    
    void GetStereoOutput(float input, float* left, float* right, bool sync_trigger = false)
    {
        float mono = Process(input, sync_trigger);
        
        // Apply panning
        float pan_rad = pan * HALFPI_F;
        *left = mono * cosf(pan_rad);
        *right = mono * sinf(pan_rad);
    }
    
    // Control methods
    void TriggerRecord() { looper.TrigRecord(); }
    void Clear() { 
        looper.Clear(); 
        has_content = false;
        loop_length = 0;
    }
    
    bool IsRecording() { return looper.Recording(); }
    bool IsRecordingQueued() { return looper.RecordingQueued(); }
    bool HasContent() { return has_content; }
    
    void SetVolume(float vol) { volume = fclamp(vol, 0.0f, 2.0f); }
    void SetPan(float p) { pan = fclamp(p, -1.0f, 1.0f); }
    void ToggleMute() { muted = !muted; }
    void ToggleSolo() { solo = !solo; }
    void SetMode(Looper::Mode mode) { looper.SetMode(mode); }
    void ToggleReverse() { 
        reverse = !reverse; 
        looper.SetReverse(reverse);
    }
    void ToggleHalfSpeed() { 
        half_speed = !half_speed; 
        looper.SetHalfSpeed(half_speed);
    }
    void SetPitchShift(float ratio) { pitch_shift = fclamp(ratio, 0.25f, 4.0f); }
    void SetFilterCutoff(float freq) { filter_cutoff = fclamp(freq, 50.0f, 8000.0f); }
    void SetFeedback(float amount) { feedback_amount = fclamp(amount, 0.0f, 0.8f); }
    
    // Performance mode controls
    void SetPerformanceMode(bool enabled) { performance_mode = enabled; }
    void SetCrossfadeTarget(float target) { crossfade_target = fclamp(target, 0.0f, 1.0f); }
};

// Global objects
static AdvancedLooperTrack tracks[NUM_TRACKS];

// Enhanced control interface
static Switch record_buttons[NUM_TRACKS];
static Switch mode_buttons[NUM_TRACKS];
static Switch mute_buttons[NUM_TRACKS];
static Switch solo_buttons[NUM_TRACKS];
static Switch reverse_buttons[NUM_TRACKS];
static Switch half_speed_buttons[NUM_TRACKS];

static Switch clear_button;
static Switch master_record;
static Switch performance_mode_button;
static Switch sync_button;

// Analog controls
static AnalogControl volume_controls[NUM_TRACKS];
static AnalogControl pan_controls[NUM_TRACKS];
static AnalogControl pitch_controls[NUM_TRACKS];
static AnalogControl filter_controls[NUM_TRACKS];
static AnalogControl feedback_controls[NUM_TRACKS];
static AnalogControl master_volume_control;
static AnalogControl tempo_control;

// Global state
static bool master_recording = false;
static bool performance_mode = false;
static bool sync_enabled = true;
static float master_volume = 1.0f;
static bool any_solo_active = false;
static float tempo_multiplier = 1.0f;

// Sync and timing
static uint32_t master_clock = 0;
static uint32_t beat_length = SAMPLE_RATE; // 1 second default
static bool sync_trigger = false;

// Advanced LED patterns
static uint32_t led_timer = 0;
static bool led_state = false;
static int led_pattern = 0;

void InitControls()
{
    // Record buttons (pins 15-18)
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        record_buttons[i].Init(hw.GetPin(15 + i), 1000.0f);
        mode_buttons[i].Init(hw.GetPin(19 + i), 1000.0f);
        mute_buttons[i].Init(hw.GetPin(23 + i), 1000.0f);
        solo_buttons[i].Init(hw.GetPin(27 + i), 1000.0f);
        reverse_buttons[i].Init(hw.GetPin(31 + i), 1000.0f);
        half_speed_buttons[i].Init(hw.GetPin(35 + i), 1000.0f);
    }
    
    // Master controls
    clear_button.Init(hw.GetPin(39), 1000.0f);
    master_record.Init(hw.GetPin(40), 1000.0f);
    performance_mode_button.Init(hw.GetPin(41), 1000.0f);
    sync_button.Init(hw.GetPin(42), 1000.0f);
    
    // Initialize ADC for analog controls
    AdcChannelConfig adc_config[22];
    for(int i = 0; i < 22; i++)
    {
        adc_config[i].InitSingle(hw.GetPin(A0 + i));
    }
    
    hw.adc.Init(adc_config, 22);
    
    // Volume controls (A0-A3)
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        volume_controls[i].Init(hw.adc.GetPtr(i), hw.AudioCallbackRate());
        pan_controls[i].Init(hw.adc.GetPtr(i + 4), hw.AudioCallbackRate());
        pitch_controls[i].Init(hw.adc.GetPtr(i + 8), hw.AudioCallbackRate());
        filter_controls[i].Init(hw.adc.GetPtr(i + 12), hw.AudioCallbackRate());
        feedback_controls[i].Init(hw.adc.GetPtr(i + 16), hw.AudioCallbackRate());
    }
    
    master_volume_control.Init(hw.adc.GetPtr(20), hw.AudioCallbackRate());
    tempo_control.Init(hw.adc.GetPtr(21), hw.AudioCallbackRate());
}

void UpdateControls()
{
    // Update analog controls
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        tracks[i].SetVolume(volume_controls[i].Process() * 2.0f);
        tracks[i].SetPan((pan_controls[i].Process() - 0.5f) * 2.0f);
        
        // Pitch control: 0.25x to 4x speed
        float pitch_val = pitch_controls[i].Process();
        float pitch_ratio = 0.25f + pitch_val * 3.75f;
        tracks[i].SetPitchShift(pitch_ratio);
        
        // Filter: 50Hz to 8kHz
        float filter_val = filter_controls[i].Process();
        float filter_freq = 50.0f + filter_val * 7950.0f;
        tracks[i].SetFilterCutoff(filter_freq);
        
        tracks[i].SetFeedback(feedback_controls[i].Process());
    }
    
    master_volume = master_volume_control.Process() * 2.0f;
    tempo_multiplier = 0.25f + tempo_control.Process() * 3.75f;
    
    // Update button states
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        record_buttons[i].Debounce();
        mode_buttons[i].Debounce();
        mute_buttons[i].Debounce();
        solo_buttons[i].Debounce();
        reverse_buttons[i].Debounce();
        half_speed_buttons[i].Debounce();
        
        // Handle button presses
        if(record_buttons[i].RisingEdge())
        {
            tracks[i].TriggerRecord();
        }
        
        if(mode_buttons[i].RisingEdge())
        {
            tracks[i].looper.IncrementMode();
        }
        
        if(mute_buttons[i].RisingEdge())
        {
            tracks[i].ToggleMute();
        }
        
        if(solo_buttons[i].RisingEdge())
        {
            tracks[i].ToggleSolo();
        }
        
        if(reverse_buttons[i].RisingEdge())
        {
            tracks[i].ToggleReverse();
        }
        
        if(half_speed_buttons[i].RisingEdge())
        {
            tracks[i].ToggleHalfSpeed();
        }
    }
    
    // Master controls
    clear_button.Debounce();
    master_record.Debounce();
    performance_mode_button.Debounce();
    sync_button.Debounce();
    
    if(clear_button.RisingEdge())
    {
        for(int i = 0; i < NUM_TRACKS; i++)
        {
            tracks[i].Clear();
        }
    }
    
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
    
    if(performance_mode_button.RisingEdge())
    {
        performance_mode = !performance_mode;
        for(int i = 0; i < NUM_TRACKS; i++)
        {
            tracks[i].SetPerformanceMode(performance_mode);
        }
    }
    
    if(sync_button.RisingEdge())
    {
        sync_enabled = !sync_enabled;
    }
    
    // Update solo state
    any_solo_active = false;
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        if(tracks[i].solo)
        {
            any_solo_active = true;
            break;
        }
    }
}

void UpdateSync()
{
    // Generate sync triggers based on tempo
    static uint32_t last_sync = 0;
    uint32_t sync_interval = static_cast<uint32_t>(beat_length / tempo_multiplier);
    
    if(sync_enabled && (master_clock - last_sync) >= sync_interval)
    {
        sync_trigger = true;
        last_sync = master_clock;
    }
    else
    {
        sync_trigger = false;
    }
    
    master_clock++;
}

void UpdateAdvancedLED()
{
    uint32_t now = System::GetNow();
    int recording_tracks = 0;
    int active_tracks = 0;
    int solo_tracks = 0;
    
    // Count track states
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        if(tracks[i].IsRecording()) recording_tracks++;
        if(tracks[i].HasContent()) active_tracks++;
        if(tracks[i].solo) solo_tracks++;
    }
    
    // Complex LED patterns
    if(performance_mode)
    {
        // Rainbow pattern in performance mode
        if(now - led_timer > 50)
        {
            led_pattern = (led_pattern + 1) % 16;
            led_state = (led_pattern % 4) < 2;
            led_timer = now;
        }
    }
    else if(recording_tracks > 0)
    {
        // Multi-phase blink for recording
        uint32_t phase = (now / 100) % (recording_tracks + 2);
        led_state = phase < recording_tracks;
    }
    else if(master_recording)
    {
        // Breathing pattern for master record
        uint32_t breath = (now / 20) % 200;
        led_state = breath < 100;
    }
    else if(solo_tracks > 0)
    {
        // Fast double-blink for solo mode
        uint32_t pattern = (now / 50) % 8;
        led_state = (pattern == 0 || pattern == 1 || pattern == 4 || pattern == 5);
    }
    else if(active_tracks > 0)
    {
        // Slow pattern based on number of active tracks
        uint32_t rate = 500 + active_tracks * 100;
        led_state = ((now / rate) % 2) == 0;
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
        // Update sync
        UpdateSync();
        
        float input_left = in[0][i];
        float input_right = in[1][i];
        float mono_input = (input_left + input_right) * 0.5f;
        
        float mixed_left = 0.0f;
        float mixed_right = 0.0f;
        int contributing_tracks = 0;
        
        // Process each track
        for(int t = 0; t < NUM_TRACKS; t++)
        {
            float track_left, track_right;
            tracks[t].GetStereoOutput(mono_input, &track_left, &track_right, sync_trigger);
            
            // Apply solo logic
            if(any_solo_active)
            {
                if(tracks[t].solo)
                {
                    mixed_left += track_left;
                    mixed_right += track_right;
                    contributing_tracks++;
                }
            }
            else
            {
                mixed_left += track_left;
                mixed_right += track_right;
                contributing_tracks++;
            }
        }
        
        // Intelligent mixing with soft limiting
        if(contributing_tracks > 0)
        {
            // Soft normalization
            float mix_level = 1.0f / sqrtf(contributing_tracks);
            mixed_left *= mix_level;
            mixed_right *= mix_level;
        }
        
        // Apply master volume
        mixed_left *= master_volume;
        mixed_right *= master_volume;
        
        // Add subtle dry monitoring
        float dry_level = 0.05f;
        mixed_left += input_left * dry_level;
        mixed_right += input_right * dry_level;
        
        // Soft limiting
        mixed_left = tanhf(mixed_left * 0.8f);
        mixed_right = tanhf(mixed_right * 0.8f);
        
        out[0][i] = mixed_left;
        out[1][i] = mixed_right;
    }
}

int main(void)
{
    // Initialize hardware
    hw.Configure();
    hw.Init();
    hw.SetAudioBlockSize(4);
    
    float sample_rate = hw.AudioSampleRate();
    
    // Initialize tracks
    for(int i = 0; i < NUM_TRACKS; i++)
    {
        tracks[i].Init(sample_rate);
        // Set different default modes for creative workflow
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
    
    // Start ADC
    hw.adc.Start();
    
    // Start audio processing
    hw.StartAudio(AudioCallback);
    
    // Main loop
    while(1)
    {
        UpdateControls();
        UpdateAdvancedLED();
        System::Delay(1);
    }
}