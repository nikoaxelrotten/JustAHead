#pragma once
#include <juce_dsp/juce_dsp.h>
#include "Dsp.h"
#include <algorithm>
#include <cmath>

// Motor de sonido de JustAHead: pedales + head, todo en mono, sin cabina.
// Las etapas no lineales corren sobremuestreadas 4x; los filtros trabajan en double.
namespace jah
{
// Interpola una tabla de n puntos repartidos en t = 0..1
inline double interpTable (const double* tbl, int n, double t)
{
    t = clampd (t, 0.0, 1.0) * (n - 1);
    const int i = (int) t;
    const int j = std::min (i + 1, n - 1);
    return tbl[i] + (tbl[j] - tbl[i]) * (t - i);
}

// Curva suave, impar y acotada en +-1, pendiente 1 en el origen. k alto = recorte mas duro.
inline double shape (double x, double k)
{
    const double ax = std::abs (x);
    if (k == 2.0) return x / std::sqrt (1.0 + ax * ax);
    return x / std::pow (1.0 + std::pow (ax, k), 1.0 / k);
}

//==============================================================================
struct Biquad
{
    double b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0, z1 = 0, z2 = 0;
    double process (double x)
    {
        const double y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
    void reset() { z1 = z2 = 0.0; }

    void setPeak (double f, double q, double gDb, double fs)
    {
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * clampd (f, 10.0, fs * 0.45) / fs;
        const double al = std::sin (w) / (2 * q), c = std::cos (w);
        norm (1 + al * A, -2 * c, 1 - al * A, 1 + al / A, -2 * c, 1 - al / A);
    }
    void setLowShelf (double f, double gDb, double fs)
    {
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * clampd (f, 10.0, fs * 0.45) / fs;
        const double c = std::cos (w), beta = 2 * std::sqrt (A) * (std::sin (w) / (2 * 0.7071));
        norm (A * ((A + 1) - (A - 1) * c + beta), 2 * A * ((A - 1) - (A + 1) * c), A * ((A + 1) - (A - 1) * c - beta),
              (A + 1) + (A - 1) * c + beta, -2 * ((A - 1) + (A + 1) * c), (A + 1) + (A - 1) * c - beta);
    }
    void setHighShelf (double f, double gDb, double fs)
    {
        const double A = std::pow (10.0, gDb / 40.0), w = 2 * kPi * clampd (f, 10.0, fs * 0.45) / fs;
        const double c = std::cos (w), beta = 2 * std::sqrt (A) * (std::sin (w) / (2 * 0.7071));
        norm (A * ((A + 1) + (A - 1) * c + beta), -2 * A * ((A - 1) + (A + 1) * c), A * ((A + 1) + (A - 1) * c - beta),
              (A + 1) - (A - 1) * c + beta, 2 * ((A - 1) - (A + 1) * c), (A + 1) - (A - 1) * c - beta);
    }

private:
    void norm (double B0, double B1, double B2, double A0, double A1, double A2)
    {
        b0 = B0 / A0; b1 = B1 / A0; b2 = B2 / A0; a1 = A1 / A0; a2 = A2 / A0;
    }
};

//==============================================================================
// Etapa de valvula: acople (HP) -> ganancia -> curva asimetrica -> filtro pasabajos (Miller).
struct TubeStage
{
    double gain = 1, k = 2, bias = 0, norm = 1, off = 0;
    OnePole hp, lp;

    void setup (double g, double k_, double b, double hpHz, double lpHz, double fs)
    {
        gain = g; k = k_; bias = b;
        off = shape (bias, k);
        const double e = 1e-4;
        norm = 1.0 / std::max (0.2, (shape (bias + e, k) - shape (bias - e, k)) / (2 * e));
        hp.set (hpHz, fs);
        lp.set (lpHz, fs);
    }
    double process (double x)
    {
        x = hp.hp (x);
        return lp.lp ((shape (x * gain + bias, k) - off) * norm);
    }
    void reset() { hp.reset(); lp.reset(); }
};

//==============================================================================
struct Voicing
{
    const char* name;
    int nStages;
    double inHP;
    double base[4], k[4], bias[4], coupHP[4], millerLP[4], w[4];
    int tsAfter;                       // el tone stack va despues de esta etapa
    double gainMinDb, gainMaxDb;       // rango de la perilla Gain
    double preShelfHz, preShelfDb, prePeakHz, prePeakDb, prePeakQ;   // color fijo antes de la 1a etapa
    double bassF, midF, midQ, trebF;   // frecuencias del EQ
    double dipHz, dipDb, dipQ;         // perdida fija del tone stack
    double pwrDrive, pwrK, sag, presF; // etapa de potencia
    double outLP;                      // pasabajos de salida
    double trimDb;
    double levelComp[5];               // compensa nivel segun Gain (0, 2.5, 5, 7.5, 10)
};

inline const Voicing& voicingFor (int amp)
{
    static const Voicing v[4] = {
        // British OD (tipo JCM800 / Zakk Wylde): 3 etapas, EQ entre la 2a y la 3a
        { "British OD", 3, 70.0,
          { 12, 8, 3, 1 }, { 2.2, 2.5, 3.0, 2 }, { 0.25, 0.2, 0.1, 0 }, { 25, 50, 40, 20 }, { 9000, 8000, 9000, 9000 }, { 0.6, 0.4, 0.0, 0 },
          1, -48.0, 0.0,
          0, 0, 0, 0, 1,
          100, 650, 0.8, 2800,
          480, -5.0, 0.7,
          1.6, 2.0, 0.25, 4200,
          11000, 0.0, { 5.9, -0.1, -8.0, -13.3, -15.2 } },
        // Modern 3Ch (tipo 5150): 4 etapas, entrada ajustada, EQ despues de la ganancia
        { "Modern 3Ch", 4, 85.0,
          { 14, 10, 8, 6 }, { 2.5, 3.0, 3.5, 4.0 }, { 0.2, 0.2, 0.15, 0.1 }, { 30, 80, 100, 120 }, { 8500, 7500, 7000, 6500 }, { 0.3, 0.3, 0.25, 0.15 },
          3, -60.0, -2.0,
          0, 0, 0, 0, 1,
          150, 700, 0.7, 3200,
          500, -3.0, 0.7,
          0.55, 2.6, 0.10, 5000,
          12500, 0.0, { 2.9, -2.6, -7.6, -9.5, -9.7 } },
        // UK Iommi (tipo Laney): oscuro y gordo, joroba en medios, 3 etapas
        { "UK Iommi", 3, 50.0,
          { 16, 9, 4, 1 }, { 2.0, 2.2, 2.5, 2 }, { 0.3, 0.3, 0.2, 0 }, { 25, 45, 35, 20 }, { 8000, 7000, 8000, 9000 }, { 0.55, 0.45, 0.0, 0 },
          1, -48.0, 2.0,
          110, 3.0, 700, 4.0, 1.0,
          110, 700, 0.8, 2400,
          420, -4.0, 0.7,
          2.0, 2.0, 0.35, 3800,
          9500, 0.0, { -2.4, -7.3, -12.6, -15.2, -15.9 } },
        // American Thick (tipo Mesa): 4 etapas, graves densos, medios recortados, mucho sag
        { "American Thick", 4, 50.0,
          { 12, 9, 7, 4 }, { 2.2, 3.0, 3.0, 4.0 }, { 0.35, 0.2, 0.2, 0.1 }, { 20, 60, 90, 100 }, { 9000, 8000, 7500, 7000 }, { 0.3, 0.3, 0.25, 0.15 },
          3, -56.0, -2.0,
          90, 4.0, 600, -5.0, 0.6,
          150, 750, 0.7, 3000,
          450, -3.0, 0.7,
          0.55, 2.4, 0.35, 5000,
          11500, 0.0, { 6.4, 0.8, -5.1, -7.5, -8.1 } },
    };
    return v[clampd (amp, 0, 3) == amp ? amp : 0];
}

//==============================================================================
struct Params
{
    int amp = 0, pedal = 0;                                  // pedal: 0 Off, 1 Overdrive, 2 Distortion
    int odMod = 0;                                           // mod del overdrive: 0 Stock, 1 Tight, 2 Hot, 3 Clean, 4 Mid+
    float gain = 6, bass = 5, mid = 5, treble = 5, presence = 5;   // 0..10
    float drive = 5, ptone = 5, levelDb = 0, masterDb = 0;
};

//==============================================================================
class PedalFx
{
public:
    // Mods del overdrive (tipo Tube Screamer): frecuencia del filtro de medios, corte de graves a la salida,
    // curva de recorte, asimetria (diodos) y ganancia por unidad de Drive.
    struct OdMod { double split, outHP, C, k, bias, dbPerDrive, toneMul, trimDb; };
    static const OdMod& odModFor (int m)
    {
        static const OdMod v[5] = {
            //  split  outHP   C     k    bias  dB/dr  tone   trim
            {   720.0,  60.0, 0.45, 1.8, 0.00, 4.0,  1.00,  0.0 },   // Stock: Tube Screamer clasico
            {  1100.0, 120.0, 0.45, 1.8, 0.00, 4.0,  1.00,  0.9 },   // Tight: graves recortados, para metal
            {   720.0,  60.0, 0.38, 3.0, 0.22, 4.6,  1.15, -0.3 },   // Hot: diodos LED, recorte duro y asimetrico
            {   250.0,  40.0, 0.80, 1.1, 0.00, 2.4,  1.20, -0.6 },   // Clean: boost casi limpio de rango completo
            {   520.0,  70.0, 0.45, 1.9, 0.00, 4.2,  0.85, -0.5 }    // Mid+: joroba de medios mas ancha
        };
        return v[m < 0 ? 0 : (m > 4 ? 4 : m)];
    }

    void setup (int t, double drive, double tone, double levelDb, double fs, int odMod = 0)
    {
        type = t;
        if (t == 1)
        {
            const OdMod& o = odModFor (odMod);
            G = dbToLin (drive * o.dbPerDrive);
            split.set (o.split, fs);
            toneLP.set (700.0 * o.toneMul * std::pow (10.0, tone * 0.1), fs);
            outHP.set (o.outHP, fs);
            C = o.C; k = o.k; bias = o.bias; off = shape (bias, k);
            comp = interpTable (odComp, 3, drive / 10.0) + o.trimDb;
        }
        else if (t == 2)
        {
            G = dbToLin (10.0 + drive * 5.0);
            inHP.set (90.0, fs);
            preLP.set (clampd (3.0e6 / G, 2500.0, 16000.0), fs);
            toneLP.set (900.0 * std::pow (10.0, tone * 0.113), fs);
            outHP.set (160.0, fs);
            C = 0.5; k = 4.5; bias = 0; off = 0;
            comp = interpTable (distComp, 3, drive / 10.0);
        }
        level = dbToLin (levelDb + comp);
    }

    double process (double x)
    {
        if (type == 1)
        {
            const double xl = split.lp (x), xh = x - xl;
            double y = xl + G * xh;
            y = C * (shape (y / C + bias, k) - off);
            y = outHP.hp (toneLP.lp (y));
            return y * level;
        }
        if (type == 2)
        {
            double y = preLP.lp (inHP.hp (x) * G);
            y = C * shape (y / C, k);
            y = outHP.hp (toneLP.lp (y));
            return y * level;
        }
        return x;
    }

    void reset() { split.reset(); toneLP.reset(); outHP.reset(); inHP.reset(); preLP.reset(); }

    // Compensacion de nivel (dB) a Drive 0, 5 y 10. Calibrada con las pruebas.
    double odComp[3]   = { 0.4, 0.7, -0.5 };
    double distComp[3] = { -1.4, -1.9, -0.8 };

private:
    int type = 0;
    double G = 1, level = 1, C = 0.45, k = 1.8, comp = 0, bias = 0, off = 0;
    OnePole split, toneLP, outHP, inHP, preLP;
};

//==============================================================================
class Engine
{
public:
    static constexpr int kSub = 64;

    void prepare (double sampleRate, int /*maxBlock*/)
    {
        fs = sampleRate;
        fsOs = sampleRate * 4.0;
        os.initProcessing ((size_t) kSub);
        os.reset();
        resetStates();
        sm = tgt;
        started = false;
    }

    int getLatency() const { return (int) std::lround (os.getLatencyInSamples()); }

    void setParams (const Params& p)
    {
        if (started && (p.amp != tgt.amp || p.pedal != tgt.pedal || p.odMod != tgt.odMod))
            fadeLeft = kFade;
        tgt = p;
    }

    void process (float* x, int n)
    {
        if (! started)
        {
            sm = tgt; started = true;
            resetStates();
        }

        int pos = 0;
        while (pos < n)
        {
            const int m = std::min (kSub, n - pos);
            const double a = 1.0 - std::exp (-(double) m / (fs * 0.02));
            auto sg = [a] (float& cur, float t) { cur += (t - cur) * (float) a; };
            sg (sm.gain, tgt.gain); sg (sm.bass, tgt.bass); sg (sm.mid, tgt.mid); sg (sm.treble, tgt.treble);
            sg (sm.presence, tgt.presence); sg (sm.drive, tgt.drive); sg (sm.ptone, tgt.ptone);
            sg (sm.levelDb, tgt.levelDb); sg (sm.masterDb, tgt.masterDb);
            sm.amp = tgt.amp; sm.pedal = tgt.pedal; sm.odMod = tgt.odMod;

            if (sm.amp != curAmp || sm.pedal != curPedal)
            {
                curAmp = sm.amp; curPedal = sm.pedal;
                resetStates();
            }
            updateCoeffs();

            float* ptrs[1] = { x + pos };
            juce::dsp::AudioBlock<float> blk (ptrs, 1, (size_t) m);
            auto up = os.processSamplesUp (blk);
            float* d = up.getChannelPointer (0);
            for (size_t i = 0; i < up.getNumSamples(); ++i)
                d[i] = (float) processSample (d[i]);
            os.processSamplesDown (blk);

            for (int i = 0; i < m; ++i)
            {
                double v = outLP2.lp (outLP1.lp ((double) x[pos + i])) * trimLin;
                if (fadeLeft > 0)
                {
                    v *= 1.0 - (double) fadeLeft / kFade;
                    --fadeLeft;
                }
                x[pos + i] = (float) v;
            }
            pos += m;
        }
    }

    Voicing vo = voicingFor (0);
    PedalFx pedal;

private:
    static constexpr int kFade = 480;

    void resetStates()
    {
        for (auto& s : stage) s.reset();
        inHP.reset(); dcHP.reset(); outLP1.reset(); outLP2.reset();
        preShelf.reset(); prePeak.reset(); dip.reset(); bassS.reset(); midP.reset(); trebS.reset(); presS.reset();
        pedal.reset();
        env = 0.0;
        fadeLeft = started ? kFade : 0;
    }

    void updateCoeffs()
    {
        vo = voicingFor (sm.amp);
        const double g = clampd (sm.gain / 10.0, 0.0, 1.0);
        const double t = std::pow (g, 1.4);
        const double gainDb = vo.gainMinDb + (vo.gainMaxDb - vo.gainMinDb) * t;
        for (int s = 0; s < vo.nStages; ++s)
            stage[s].setup (vo.base[s] * dbToLin (vo.w[s] * gainDb), vo.k[s], vo.bias[s], vo.coupHP[s], vo.millerLP[s], fsOs);

        inHP.set (vo.inHP, fsOs);
        preShelf.setLowShelf (vo.preShelfHz > 0 ? vo.preShelfHz : 100.0, vo.preShelfDb, fsOs);
        prePeak.setPeak (vo.prePeakHz > 0 ? vo.prePeakHz : 700.0, vo.prePeakQ, vo.prePeakDb, fsOs);
        dip.setPeak (vo.dipHz, vo.dipQ, vo.dipDb, fsOs);
        bassS.setLowShelf (vo.bassF, (sm.bass - 5.0) * 3.2, fsOs);
        midP.setPeak (vo.midF, vo.midQ, (sm.mid - 5.0) * 2.4, fsOs);
        trebS.setHighShelf (vo.trebF, (sm.treble - 5.0) * 2.8, fsOs);
        presS.setHighShelf (vo.presF, (sm.presence - 5.0) * 2.4, fsOs);
        sagAtk = 1.0 - std::exp (-1.0 / (fsOs * 0.004));
        sagRel = 1.0 - std::exp (-1.0 / (fsOs * 0.15));
        dcHP.set (20.0, fsOs);

        pedal.setup (sm.pedal, sm.drive, sm.ptone, sm.levelDb, fsOs, sm.odMod);

        outLP1.set (vo.outLP, fs);
        outLP2.set (vo.outLP, fs);
        trimLin = dbToLin (vo.trimDb + interpTable (vo.levelComp, 5, g));
    }

    double processSample (double x)
    {
        double v = x;
        if (sm.pedal != 0) v = pedal.process (v);
        v = inHP.hp (v);
        if (vo.preShelfDb != 0.0) v = preShelf.process (v);
        if (vo.prePeakDb != 0.0) v = prePeak.process (v);
        for (int s = 0; s < vo.nStages; ++s)
        {
            v = stage[s].process (v);
            if (s == vo.tsAfter)
                v = trebS.process (midP.process (bassS.process (dip.process (v))));
        }
        v = presS.process (v);
        const double a = std::abs (v);
        env += (a > env ? sagAtk : sagRel) * (a - env);
        v /= 1.0 + vo.sag * env;
        v = shape (v * vo.pwrDrive, vo.pwrK);
        return dcHP.hp (v);
    }

    double fs = 48000.0, fsOs = 192000.0;
    juce::dsp::Oversampling<float> os { 1, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, false };
    Params tgt, sm;
    bool started = false;
    int curAmp = -1, curPedal = -1, fadeLeft = 0;

    TubeStage stage[4];
    OnePole inHP, dcHP, outLP1, outLP2;
    Biquad preShelf, prePeak, dip, bassS, midP, trebS, presS;
    double env = 0, sagAtk = 0.01, sagRel = 0.0001, trimLin = 1.0;
};
} // namespace jah
