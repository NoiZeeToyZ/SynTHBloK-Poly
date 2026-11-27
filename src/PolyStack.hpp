#pragma once
// SynTHBloK-POLY's oscillator stack (Nov 2026 "lush" pass). No Rack headers, so tools/sound_check/ renders it.
//
// Each stacked interval runs its own phase and is a band-limited (PolyBLEP) saw, a few cents off the others and
// drifting, so a stack sounds like several real oscillators. Before, every interval was worked out from the
// voice's one phase (phase x ratio, wrapped): any interval that is not a whole octave (fifths, thirds, 19, 21)
// jumped back to the start of its wave once per cycle of the root, a hard click at the note's pitch, and none
// of the saws were band-limited. That was the buzzy, 8-bit edge. The pad sines (half and 1.5x the root) had the
// same per-cycle kink; they get their own phases too.
#include <cmath>
#include <cstdint>

namespace {

struct PolyStackOsc {
    static constexpr int MAX = 5;
    float ph[MAX] = {};
    float bodyPh = 0.f, fifthPh = 0.f;
    float drift[MAX] = {}, driftTarget[MAX] = {}, driftTimer[MAX] = {};
    uint32_t rng = 1;

    float rand01() { rng = rng * 1664525u + 1013904223u; return (rng >> 8) * (1.f / 16777216.f); }
    void seed(uint32_t s) {
        rng = s * 2654435761u + 12345u;
        for (int i = 0; i < MAX; i++) ph[i] = rand01();   // free-running: the copies start out of phase
        bodyPh = rand01(); fifthPh = rand01();
    }
    static float blep(float t, float dt) {
        if (t < dt) { t /= dt; return t + t - t * t - 1.f; }
        if (t > 1.f - dt) { t = (t - 1.f) / dt; return t * t + t + t + 1.f; }
        return 0.f;
    }

    // basePhase: the voice's root phase (0..1) and inc its step per sample. iv / n: the stack's intervals.
    // shape / sat: the voice's analog bias. Outputs the stack (about +-1), and the two pad sines.
    void process(float basePhase, float inc, const int* iv, int n, float shape, float sat, float sr,
                 float& osc, float& sineBody, float& fifth) {
        float sum = 0.f;
        for (int i = 0; i < n; i++) {
            // each copy wanders up to +-2.5 cents, and copies past the root sit +-4 cents apart (alternating)
            driftTimer[i] -= 1.f / sr;
            if (driftTimer[i] <= 0.f) { driftTarget[i] = rand01() * 2.f - 1.f; driftTimer[i] = 0.4f + rand01(); }
            drift[i] += (driftTarget[i] - drift[i]) * (1.5f / sr);
            float cents = drift[i] * 2.5f + (i == 0 ? 0.f : (i & 1 ? 4.f : -4.f));
            float ratio = std::exp2((iv[i] * 100.f + cents) / 1200.f);
            float dt = std::min(inc * ratio, 0.45f);
            float p = ph[i] + dt;
            p -= std::floor(p);
            ph[i] = p;
            float o = (2.f * p - 1.f - blep(p, dt)) * shape;
            float s = std::tanh(o * sat);
            sum += 0.72f * o + 0.28f * s;
        }
        osc = sum / (n > 0 ? n : 1);
        bodyPh += 0.5f * inc; bodyPh -= std::floor(bodyPh);
        fifthPh += 1.5f * inc; fifthPh -= std::floor(fifthPh);
        sineBody = std::sin(bodyPh * 2.f * float(M_PI));
        fifth = std::sin(fifthPh * 2.f * float(M_PI));
        (void)basePhase;
    }
};

} // namespace
