#include "Utility/looper.h"
#include "audio.h"
#include "daisysp.h"
#include "daisy_pod.h"
#include "Effects/pitchshifter.h"
#include <cstddef>
#include <type_traits>

using namespace daisysp;
using namespace daisy;

#define NUM_TRACKS 1
#define SAMPLE_RATE 48000
#define MAX_LOOP_TIME 120.0f
#define BUFFER_SIZE static_cast<uint32_t>(SAMPLE_RATE * MAX_LOOP_TIME)

enum TrackMode
{
    MODE_LOOP   = 0,
    MODE_SINGLE = 1
};

enum TrackState
{
    STATE_STOPPED     = 0,
    STATE_PLAYING     = 1,
    STATE_RECORDING   = 2,
    STATE_OVERDUBBING = 3
};

class TapeTrack
{
  public:
    TrackMode  mode;
    TrackState state;
    Looper     loop;
    float      buffer[BUFFER_SIZE];
    float      volume = 1.0f;
    float      pan    = 0.0f;
    float      speed  = 1.0f;

    void Init()
    {
        mode  = TrackMode::MODE_LOOP;
        state = TrackState::STATE_STOPPED;
        loop.Init(buffer, BUFFER_SIZE);
    }

    void Process() {}
};

static DaisyPod pod;
TapeTrack       tracks[NUM_TRACKS];

int main(void)
{
    for(auto track : tracks)
    {
        track.Init();
    }

    pod.Init();
    pod.SetAudioBlockSize(4);
}

void AudioCallback(AudioHandle::InputBuffer  in,
                   AudioHandle::OutputBuffer out,
                   size_t                    size)
{
}
