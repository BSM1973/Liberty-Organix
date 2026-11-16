#include "../Source/MaterialDSP.h"
#include <cmath>
#include <iostream>
#include <limits>

int main()
{
    constexpr double rate = 48000.0;
    constexpr int samples = 10000;
    for (int material = 0; material < 6; ++material)
    {
        for (int control = 0; control < 12; ++control)
        {
            organix::Parameters low, high;
            low.amount = high.amount = 0.68f;
            low.motion = high.motion = 0.55f;
            low.tone = high.tone = 0.62f;
            low.feedback = high.feedback = 0.40f;
            low.shape[static_cast<size_t> (control)] = 0.0f;
            high.shape[static_cast<size_t> (control)] = 1.0f;
            organix::MaterialDSP a, b;
            a.prepare (rate, 1);
            b.prepare (rate, 1);
            a.configure (static_cast<organix::Material> (material), low);
            b.configure (static_cast<organix::Material> (material), high);
            double totalDifference = 0.0;
            for (int n = 0; n < samples; ++n)
            {
                const float time = static_cast<float> (n / rate);
                float in = 0.21f * std::sin (2.0f * 3.14159265f * 220.0f * time);
                in += 0.11f * std::sin (2.0f * 3.14159265f * 1700.0f * time);
                if (n % 1301 == 0) in += 0.35f;
                const float yA = a.process (0, in), yB = b.process (0, in);
                if (! std::isfinite (yA) || ! std::isfinite (yB) ||
                    std::fabs (yA) > 4.001f || std::fabs (yB) > 4.001f)
                {
                    std::cerr << "Non-finite/unbounded DSP: material " << material
                              << ", parameter " << control << "\n";
                    return 1;
                }
                totalDifference += std::fabs (yA - yB);
            }
            if (totalDifference < 1.0e-4)
            {
                std::cerr << "Inert material control: material " << material
                          << ", parameter " << (control + 1) << "\n";
                return 2;
            }
        }
    }
    std::cout << "PASS: all 72 material/profile control positions produce bounded audible DSP changes\n";
    return 0;
}
