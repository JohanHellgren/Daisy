#include "Utility/looper.h"
#include "daisysp.h"
#include "daisy_pod.h"

using namespace daisysp;
using namespace daisy;

static DaisyPod pod;

int main(void)
{
    pod.Init();
    pod.SetAudioBlockSize(4);
}
