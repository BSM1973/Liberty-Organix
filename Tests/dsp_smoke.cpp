#include "../Source/MaterialDSP.h"
#include <cassert>
#include <cmath>
#include <iostream>

int main()
{
    for (auto rate : {44100.0, 48000.0, 96000.0, 192000.0})
    {
        for (int kind = 0; kind < 6; ++kind)
        {
            organix::MaterialDSP dsp;
            dsp.prepare (rate, 2);
            organix::Parameters p;
            p.amount = 0.72f;
            p.motion = 0.64f;
            p.feedback = 0.48f;
            dsp.configure (static_cast<organix::Material> (kind), p);
            float rms = 0.0f;
            for (int i = 0; i < static_cast<int> (rate * 0.6); ++i)
            {
                float x = 0.20f * std::sin (2.0 * 3.141592653589793 * 220.0 * i / rate);
                if (i % 997 == 0) x += 0.45f;
                for (int ch = 0; ch < 2; ++ch)
                {
                    const float y = dsp.process (ch, x);
                    assert (std::isfinite (y) && std::abs (y) <= 4.01f);
                    rms += y * y;
                }
            }
            assert (rms > 0.01f);
            dsp.reset();
        }
    }
    std::cout << "OK: six real DSP engines at 44.1/48/96/192 kHz, finite bounded output\n";
}
