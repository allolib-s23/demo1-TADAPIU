#include <cstdio> // for printing to stdout

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Gamma.h"
#include "Gamma/Oscillator.h"
#include "Gamma/Types.h"
#include "Gamma/DFT.h"

#include "al/app/al_App.hpp"
#include "al/graphics/al_Shapes.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"
#include "al/io/al_MIDI.hpp"
#include "al/math/al_Random.hpp"

// using namespace gam;
using namespace al;
using namespace std;
#define FFT_SIZE 4048

class AddSyn : public SynthVoice {
public:
  gam::Sine<> mOsc;
  gam::Sine<> mOsc1;
  gam::Sine<> mOsc2;
  gam::Sine<> mOsc3;
  gam::Sine<> mOsc4;
  gam::Sine<> mOsc5;
  gam::Sine<> mOsc6;
  gam::Sine<> mOsc7;
  gam::Sine<> mOsc8;
  gam::Sine<> mOsc9;
  gam::ADSR<> mEnvStri;
  gam::ADSR<> mEnvLow;
  gam::ADSR<> mEnvUp;
  gam::Pan<> mPan;
  gam::EnvFollow<> mEnvFollow;

  // Additional members
  Mesh mMesh;

  virtual void init() {

    // Intialize envelopes
    mEnvStri.curve(-4); // make segments lines
    mEnvStri.levels(0, 1, 1, 0);
    mEnvStri.lengths(0.1, 0.1, 0.1);
    mEnvStri.sustain(2); // Make point 2 sustain until a release is issued
    mEnvLow.curve(-4);   // make segments lines
    mEnvLow.levels(0, 1, 1, 0);
    mEnvLow.lengths(0.1, 0.1, 0.1);
    mEnvLow.sustain(2); // Make point 2 sustain until a release is issued
    mEnvUp.curve(-4);   // make segments lines
    mEnvUp.levels(0, 1, 1, 0);
    mEnvUp.lengths(0.1, 0.1, 0.1);
    mEnvUp.sustain(2); // Make point 2 sustain until a release is issued

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("amp", 0.01, 0.0, 0.3);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("ampStri", 0.5, 0.0, 1.0);
    createInternalTriggerParameter("attackStri", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseStri", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("sustainStri", 0.8, 0.0, 1.0);
    createInternalTriggerParameter("ampLow", 0.5, 0.0, 1.0);
    createInternalTriggerParameter("attackLow", 0.001, 0.01, 3.0);
    createInternalTriggerParameter("releaseLow", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("sustainLow", 0.8, 0.0, 1.0);
    createInternalTriggerParameter("ampUp", 0.6, 0.0, 1.0);
    createInternalTriggerParameter("attackUp", 0.01, 0.01, 3.0);
    createInternalTriggerParameter("releaseUp", 0.075, 0.1, 10.0);
    createInternalTriggerParameter("sustainUp", 0.9, 0.0, 1.0);
    createInternalTriggerParameter("freqStri1", 1.0, 0.1, 10);
    createInternalTriggerParameter("freqStri2", 2.001, 0.1, 10);
    createInternalTriggerParameter("freqStri3", 3.0, 0.1, 10);
    createInternalTriggerParameter("freqLow1", 4.009, 0.1, 10);
    createInternalTriggerParameter("freqLow2", 5.002, 0.1, 10);
    createInternalTriggerParameter("freqUp1", 6.0, 0.1, 10);
    createInternalTriggerParameter("freqUp2", 7.0, 0.1, 10);
    createInternalTriggerParameter("freqUp3", 8.0, 0.1, 10);
    createInternalTriggerParameter("freqUp4", 9.0, 0.1, 10);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  virtual void onProcess(AudioIOData &io) override {
    // Parameters will update values once per audio callback
    float freq = getInternalParameterValue("frequency");
    mOsc.freq(freq);
    mOsc1.freq(getInternalParameterValue("freqStri1") * freq);
    mOsc2.freq(getInternalParameterValue("freqStri2") * freq);
    mOsc3.freq(getInternalParameterValue("freqStri3") * freq);
    mOsc4.freq(getInternalParameterValue("freqLow1") * freq);
    mOsc5.freq(getInternalParameterValue("freqLow2") * freq);
    mOsc6.freq(getInternalParameterValue("freqUp1") * freq);
    mOsc7.freq(getInternalParameterValue("freqUp2") * freq);
    mOsc8.freq(getInternalParameterValue("freqUp3") * freq);
    mOsc9.freq(getInternalParameterValue("freqUp4") * freq);
    mPan.pos(getInternalParameterValue("pan"));
    float ampStri = getInternalParameterValue("ampStri");
    float ampUp = getInternalParameterValue("ampUp");
    float ampLow = getInternalParameterValue("ampLow");
    float amp = getInternalParameterValue("amp");
    while (io()) {
      float s1 = (mOsc1() + mOsc2() + mOsc3()) * mEnvStri() * ampStri;
      s1 += (mOsc4() + mOsc5()) * mEnvLow() * ampLow;
      s1 += (mOsc6() + mOsc7() + mOsc8() + mOsc9()) * mEnvUp() * ampUp;
      s1 *= amp;
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // if(mEnvStri.done()) free();
    if (mEnvStri.done() && mEnvUp.done() && mEnvLow.done() &&
        (mEnvFollow.value() < 0.001))
      free();
  }

  virtual void onProcess(Graphics &g) {
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    g.pushMatrix();
    g.translate(amplitude, amplitude, -4);
    // g.scale(frequency/2000, frequency/4000, 1);
    float scaling = 0.1;
    g.scale(scaling * frequency / 200, scaling * frequency / 400, scaling * 1);
    g.color(mEnvFollow.value(), frequency / 1000, mEnvFollow.value() * 10, 0.4);
    g.draw(mMesh);
    g.popMatrix();
  }

  virtual void onTriggerOn() override {

    mEnvStri.attack(getInternalParameterValue("attackStri"));
    mEnvStri.decay(getInternalParameterValue("attackStri"));
    mEnvStri.sustain(getInternalParameterValue("sustainStri"));
    mEnvStri.release(getInternalParameterValue("releaseStri"));

    mEnvLow.attack(getInternalParameterValue("attackLow"));
    mEnvLow.decay(getInternalParameterValue("attackLow"));
    mEnvLow.sustain(getInternalParameterValue("sustainLow"));
    mEnvLow.release(getInternalParameterValue("releaseLow"));

    mEnvUp.attack(getInternalParameterValue("attackUp"));
    mEnvUp.decay(getInternalParameterValue("attackUp"));
    mEnvUp.sustain(getInternalParameterValue("sustainUp"));
    mEnvUp.release(getInternalParameterValue("releaseUp"));

    mPan.pos(getInternalParameterValue("pan"));

    mEnvStri.reset();
    mEnvLow.reset();
    mEnvUp.reset();
  }

  virtual void onTriggerOff() override {
    //    std::cout << "trigger off" <<std::endl;
    mEnvStri.triggerRelease();
    mEnvLow.triggerRelease();
    mEnvUp.triggerRelease();
  }
};

class SquareWave : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc1;
  gam::Sine<> mOsc3;
  gam::Sine<> mOsc5;

  gam::Env<3> mAmpEnv;

  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override
  {
    // Intialize envelope
    mAmpEnv.curve(0); // make segments lines
    mAmpEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    createInternalTriggerParameter("amplitude", 0.8, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 440, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  // The audio processing function
  void onProcess(AudioIOData &io) override
  {
    // Get the values from the parameters and apply them to the corresponding
    // unit generators. You could place these lines in the onTrigger() function,
    // but placing them here allows for realtime prototyping on a running
    // voice, rather than having to trigger a new voice to hear the changes.
    // Parameters will update values once per audio callback because they
    // are outside the sample processing loop.
    float f = getInternalParameterValue("frequency");
    mOsc1.freq(f);
    mOsc3.freq(f * 3);
    mOsc5.freq(f * 5);

    float a = getInternalParameterValue("amplitude");
    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io())
    {
      float s1 = mAmpEnv() * (mOsc1() * a +
                              mOsc3() * (a / 3.0) +
                              mOsc5() * (a / 5.0));

      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done())
      free();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }
  void onTriggerOff() override { mAmpEnv.release(); }
};

class Sub : public SynthVoice {
public:

    // Unit generators
    float mNoiseMix;
    gam::Pan<> mPan;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;  // envelope follower to connect audio output to graphics
    gam::DSF<> mOsc;
    gam::NoiseWhite<> mNoise;
    gam::Reson<> mRes;
    gam::Env<2> mCFEnv;
    gam::Env<2> mBWEnv;
    // Additional members
    Mesh mMesh;

    // Initialize voice. This function will nly be called once per voice
    void init() override {
        mAmpEnv.curve(0); // linear segments
        mAmpEnv.levels(0,1.0,1.0,0); // These tables are not normalized, so scale to 0.3
        mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued
        mCFEnv.curve(0);
        mBWEnv.curve(0);
        mOsc.harmonics(12);
        // We have the mesh be a sphere
        addDisc(mMesh, 1.0, 30);

        createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
        createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
        createInternalTriggerParameter("curve", 4.0, -10.0, 10.0);
        createInternalTriggerParameter("noise", 0.0, 0.0, 1.0);
        createInternalTriggerParameter("envDur",1, 0.0, 5.0);
        createInternalTriggerParameter("cf1", 400.0, 10.0, 5000);
        createInternalTriggerParameter("cf2", 400.0, 10.0, 5000);
        createInternalTriggerParameter("cfRise", 0.5, 0.1, 2);
        createInternalTriggerParameter("bw1", 700.0, 10.0, 5000);
        createInternalTriggerParameter("bw2", 900.0, 10.0, 5000);
        createInternalTriggerParameter("bwRise", 0.5, 0.1, 2);
        createInternalTriggerParameter("hmnum", 12.0, 5.0, 20.0);
        createInternalTriggerParameter("hmamp", 1.0, 0.0, 1.0);
        createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);

    }

    //
    
    virtual void onProcess(AudioIOData& io) override {
        updateFromParameters();
        float amp = getInternalParameterValue("amplitude");
        float noiseMix = getInternalParameterValue("noise");
        while(io()){
            // mix oscillator with noise
            float s1 = mOsc()*(1-noiseMix) + mNoise()*noiseMix;

            // apply resonant filter
            mRes.set(mCFEnv(), mBWEnv());
            s1 = mRes(s1);

            // appy amplitude envelope
            s1 *= mAmpEnv() * amp;

            float s2;
            mPan(s1, s1,s2);
            io.out(0) += s1;
            io.out(1) += s2;
        }
        
        
        if(mAmpEnv.done() && (mEnvFollow.value() < 0.001f)) free();
    }

   virtual void onProcess(Graphics &g) {
          float frequency = getInternalParameterValue("frequency");
          float amplitude = getInternalParameterValue("amplitude");
          g.pushMatrix();
          g.translate(amplitude,  amplitude, -4);
          //g.scale(frequency/2000, frequency/4000, 1);
          float scaling = 0.1;
          g.scale(scaling * frequency/200, scaling * frequency/400, scaling* 1);
          g.color(mEnvFollow.value(), frequency/1000, mEnvFollow.value()* 10, 0.4);
          g.draw(mMesh);
          g.popMatrix();
   }
    virtual void onTriggerOn() override {
        updateFromParameters();
        mAmpEnv.reset();
        mCFEnv.reset();
        mBWEnv.reset();
        
    }

    virtual void onTriggerOff() override {
        mAmpEnv.triggerRelease();
//        mCFEnv.triggerRelease();
//        mBWEnv.triggerRelease();
    }

    void updateFromParameters() {
        mOsc.freq(getInternalParameterValue("frequency"));
        mOsc.harmonics(getInternalParameterValue("hmnum"));
        mOsc.ampRatio(getInternalParameterValue("hmamp"));
        mAmpEnv.attack(getInternalParameterValue("attackTime"));
    //    mAmpEnv.decay(getInternalParameterValue("attackTime"));
        mAmpEnv.release(getInternalParameterValue("releaseTime"));
        mAmpEnv.levels()[1]=getInternalParameterValue("sustain");
        mAmpEnv.levels()[2]=getInternalParameterValue("sustain");

        mAmpEnv.curve(getInternalParameterValue("curve"));
        mPan.pos(getInternalParameterValue("pan"));
        mCFEnv.levels(getInternalParameterValue("cf1"),
                      getInternalParameterValue("cf2"),
                      getInternalParameterValue("cf1"));


        mCFEnv.lengths()[0] = getInternalParameterValue("cfRise");
        mCFEnv.lengths()[1] = 1 - getInternalParameterValue("cfRise");
        mBWEnv.levels(getInternalParameterValue("bw1"),
                      getInternalParameterValue("bw2"),
                      getInternalParameterValue("bw1"));
        mBWEnv.lengths()[0] = getInternalParameterValue("bwRise");
        mBWEnv.lengths()[1] = 1- getInternalParameterValue("bwRise");

        mCFEnv.totalLength(getInternalParameterValue("envDur"));
        mBWEnv.totalLength(getInternalParameterValue("envDur"));
    }
};

// from audiovisual/09_pluck_visual.cpp
class PluckedString : public SynthVoice
{
public:
    float mAmp;
    float mDur;
    float mPanRise;
    gam::Pan<> mPan;
    gam::NoiseWhite<> noise;
    gam::Decay<> env;
    gam::MovingAvg<> fil{2};
    gam::Delay<float, gam::ipl::Trunc> delay;
    gam::ADSR<> mAmpEnv;
    gam::EnvFollow<> mEnvFollow;
    gam::Env<2> mPanEnv;
    gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::HANN, gam::MAG_FREQ);
    // This time, let's use spectrograms for each notes as the visual components.
    Mesh mSpectrogram;
    vector<float> spectrum;
    double a = 0;
    double b = 0;
    double timepose = 10;
    // Additional members
    Mesh mMesh;

    virtual void init() override
    {
        // Declare the size of the spectrum
        spectrum.resize(FFT_SIZE / 2 + 1);
        // mSpectrogram.primitive(Mesh::POINTS);
        mSpectrogram.primitive(Mesh::LINE_STRIP);
        mAmpEnv.levels(0, 1, 1, 0);
        mPanEnv.curve(4);
        env.decay(0.1);
        delay.maxDelay(1. / 27.5);
        delay.delay(1. / 440.0);

        addDisc(mMesh, 1.0, 30);
        createInternalTriggerParameter("amplitude", 0.1, 0.0, 1.0);
        createInternalTriggerParameter("frequency", 60, 20, 5000);
        createInternalTriggerParameter("attackTime", 0.001, 0.001, 1.0);
        createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
        createInternalTriggerParameter("sustain", 0.7, 0.0, 1.0);
        createInternalTriggerParameter("Pan1", 0.0, -1.0, 1.0);
        createInternalTriggerParameter("Pan2", 0.0, -1.0, 1.0);
        createInternalTriggerParameter("PanRise", 0.0, 0, 3.0); // range check
    }

    //    void reset(){ env.reset(); }

    float operator()()
    {
        return (*this)(noise() * env());
    }
    float operator()(float in)
    {
        return delay(
            fil(delay() + in));
    }

    virtual void onProcess(AudioIOData &io) override
    {

        while (io())
        {
            mPan.pos(mPanEnv());
            float s1 = (*this)() * mAmpEnv() * mAmp;
            float s2;
            mEnvFollow(s1);
            mPan(s1, s1, s2);
            io.out(0) += s1;
            io.out(1) += s2;
            // STFT for each notes
            if (stft(s1))
            { // Loop through all the frequency bins
                for (unsigned k = 0; k < stft.numBins(); ++k)
                {
                    // Here we simply scale the complex sample
                    spectrum[k] = tanh(pow(stft.bin(k).real(), 1.3));
                }
            }
        }
        if (mAmpEnv.done() && (mEnvFollow.value() < 0.001))
            free();
    }

    virtual void onProcess(Graphics &g) override
    {
        float frequency = getInternalParameterValue("frequency");
        float amplitude = getInternalParameterValue("amplitude");
        a += 0.29;
        b += 0.23;
        timepose -= 0.1;

        mSpectrogram.reset();
        // mSpectrogram.primitive(Mesh::LINE_STRIP);

        for (int i = 0; i < FFT_SIZE / 2; i++)
        {
            mSpectrogram.color(HSV(spectrum[i] * 1000 + al::rnd::uniform()));
            mSpectrogram.vertex(i, spectrum[i], 0.0);
        }
        g.meshColor(); // Use the color in the mesh
        g.pushMatrix();
        g.translate(0, 0, -10);
        g.rotate(a, Vec3f(0, 1, 0));
        g.rotate(b, Vec3f(1));
        g.scale(10.0 / FFT_SIZE, 500, 1.0);
        g.draw(mSpectrogram);
        g.popMatrix();
    }

    virtual void onTriggerOn() override
    {
        mAmpEnv.reset();
        timepose = 10;
        updateFromParameters();
        env.reset();
        delay.zero();
        mPanEnv.reset();
    }

    virtual void onTriggerOff() override
    {
        mAmpEnv.triggerRelease();
    }

    void updateFromParameters()
    {
        mPanEnv.levels(getInternalParameterValue("Pan1"),
                       getInternalParameterValue("Pan2"),
                       getInternalParameterValue("Pan1"));
        mPanRise = getInternalParameterValue("PanRise");
        delay.freq(getInternalParameterValue("frequency"));
        mAmp = getInternalParameterValue("amplitude");
        mAmpEnv.levels()[1] = 1.0;
        mAmpEnv.levels()[2] = getInternalParameterValue("sustain");
        mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
        mAmpEnv.lengths()[3] = getInternalParameterValue("releaseTime");
        mPanEnv.lengths()[0] = mPanRise;
        mPanEnv.lengths()[1] = mPanRise;
    }
};

//From https://github.com/allolib-s21/notes-Mitchell57:
class Kick : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc;
  gam::Decay<> mDecay; // Added decay envelope for pitch
  gam::AD<> mAmpEnv; // Changed amp envelope from Env<3> to AD<>

  void init() override {
    // Intialize amplitude envelope
    // - Minimum attack (to make it thump)
    // - Short decay
    // - Maximum amplitude
    mAmpEnv.attack(0.01);
    mAmpEnv.decay(0.3);
    mAmpEnv.amp(1.0);

    // Initialize pitch decay 
    mDecay.decay(0.3);

    createInternalTriggerParameter("amplitude", 0.5, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 150, 20, 5000);
  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    mOsc.freq(getInternalParameterValue("frequency"));
    mPan.pos(0);
    // (removed parameter control for attack and release)

    while (io()) {
      mOsc.freqMul(mDecay()); // Multiply pitch oscillator by next decay value
      float s1 = mOsc() *  mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }

    if (mAmpEnv.done()){ 
      free();
    }
  }

  void onTriggerOn() override { mAmpEnv.reset(); mDecay.reset(); }

  void onTriggerOff() override { mAmpEnv.release(); mDecay.finish(); }
};

//From https://github.com/allolib-s21/notes-Mitchell57:
//commented out reverbs bc I think they're in his "theory" class

class Hihat : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::AD<> mAmpEnv; // Changed amp envelope from Env<3> to AD<>
  
  gam::Burst mBurst; // Resonant noise with exponential decay

  void init() override {
    // Initialize burst - Main freq, filter freq, duration
    mBurst = gam::Burst(20000, 15000, 0.05);

  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    while (io()) {
      float s1 = mBurst();
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // Left this in because I'm not sure how to tell when a burst is done
    if (mAmpEnv.done()) free();
  }
  void onTriggerOn() override { mBurst.reset(); }
  //void onTriggerOff() override {  }
};

class Snare : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::AD<> mAmpEnv; // Amplitude envelope
  gam::Sine<> mOsc; // Main pitch osc (top of drum)
  gam::Sine<> mOsc2; // Secondary pitch osc (bottom of drum)
  gam::Decay<> mDecay; // Pitch decay for oscillators
  // gam::ReverbMS<> reverb;	// Schroeder reverberator
  gam::Burst mBurst; // Noise to simulate rattle/chains


  void init() override {
    // Initialize burst 
    mBurst = gam::Burst(8000, 5000, 0.1);
    //editing last number of burst shortens/makes sound snappier

    // Initialize amplitude envelope
    mAmpEnv.attack(0.01);
    mAmpEnv.decay(0.01);
    mAmpEnv.amp(0.005);

    // Initialize pitch decay 
    mDecay.decay(0.1);

    // reverb.resize(gam::FREEVERB);
		// reverb.decay(0.5); // Set decay length, in seconds
		// reverb.damping(0.2); // Set high-frequency damping factor in [0, 1]

  }

  // The audio processing function
  void onProcess(AudioIOData& io) override {
    mOsc.freq(200);
    mOsc2.freq(150);

    while (io()) {
      
      // Each mDecay() call moves it forward (I think), so we only want
      // to call it once per sample
      float decay = mDecay();
      mOsc.freqMul(decay);
      mOsc2.freqMul(decay);

      float amp = mAmpEnv();
      float s1 = mBurst() + (mOsc() * amp * 0.1)+ (mOsc2() * amp * 0.05);
      // s1 += reverb(s1) * 0.2;
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    
    if (mAmpEnv.done()) free();
  }
  void onTriggerOn() override { mBurst.reset(); mAmpEnv.reset(); mDecay.reset();}
  
  void onTriggerOff() override { mAmpEnv.release(); mDecay.finish(); }
};

class SineEnv : public SynthVoice {
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc;
  gam::Env<3> mAmpEnv;
  // envelope follower to connect audio output to graphics
  gam::EnvFollow<> mEnvFollow;

  // Additional members
  Mesh mMesh;

  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override {
    // Intialize envelope
    mAmpEnv.curve(0); // make segments lines
    mAmpEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    // This is a quick way to create parameters for the voice. Trigger
    // parameters are meant to be set only when the voice starts, i.e. they
    // are expected to be constant within a voice instance. (You can actually
    // change them while you are prototyping, but their changes will only be
    // stored and aplied when a note is triggered.)

    createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 1.0, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 3.0, 0.1, 10.0);
    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  // The audio processing function
  void onProcess(AudioIOData &io) override {
    // Get the values from the parameters and apply them to the corresponding
    // unit generators. You could place these lines in the onTrigger() function,
    // but placing them here allows for realtime prototyping on a running
    // voice, rather than having to trigger a new voice to hear the changes.
    // Parameters will update values once per audio callback because they
    // are outside the sample processing loop.
    mOsc.freq(getInternalParameterValue("frequency"));
    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io()) {
      float s1 = mOsc() * mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001f)){
      free();
    }
  }

  // The graphics processing function
  void onProcess(Graphics &g) override {
    // Get the paramter values on every video frame, to apply changes to the
    // current instance
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    // Now draw
    g.pushMatrix();
    // Move x according to frequency, y according to amplitude
    g.translate(frequency / 200 - 3, amplitude, -8);
    // Scale in the x and y directions according to amplitude
    g.scale(1 - amplitude, amplitude, 1);
    // Set the color. Red and Blue according to sound amplitude and Green
    // according to frequency. Alpha fixed to 0.4
    g.color(mEnvFollow.value(), frequency / 1000, mEnvFollow.value() * 10, 0.4);
    g.draw(mMesh);
    g.popMatrix();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }

  void onTriggerOff() override { mAmpEnv.release(); }
};

// We make an app.
class MyApp : public App {
public:
  // GUI manager for SineEnv voices
  // The name provided determines the name of the directory
  // where the presets and sequences are stored
  SynthGUIManager<SquareWave> synthManager{"SquareWave"};

  // This function is called right after the window is created
  // It provides a grphics context to initialize ParameterGUI
  // It's also a good place to put things that should
  // happen once at startup.
  void onCreate() override {
    navControl().active(false); // Disable navigation via keyboard, since we
                                // will be using keyboard for note triggering

    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());

    imguiInit();

    // Play example sequence. Comment this line to start from scratch
    playTune();
    // synthManager.synthSequencer().playSequence("synth1.synthSequence");
    synthManager.synthRecorder().verbose(true);
  }

  // The audio callback function. Called when audio hardware requires data
  void onSound(AudioIOData &io) override {
    synthManager.render(io); // Render audio
  }

  void onAnimate(double dt) override {
    // The GUI is prepared here
    imguiBeginFrame();
    // Draw a window that contains the synth control panel
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
  }

  // The graphics callback function.
  void onDraw(Graphics &g) override {
    g.clear();
    // Render the synth's graphics
    synthManager.render(g);

    // GUI is drawn here
    imguiDraw();
  }

  // Whenever a key is pressed, this function is called
  bool onKeyDown(Keyboard const &k) override {
    if (ParameterGUI::usingKeyboard()) { // Ignore keys if GUI is using
                                         // keyboard
      return true;
    }
    if (k.shift()) {
      // If shift pressed then keyboard sets preset
      int presetNumber = asciiToIndex(k.key());
      synthManager.recallPreset(presetNumber);
    } else {
      // Otherwise trigger note for polyphonic synth
      int midiNote = asciiToMIDI(k.key());
      if (midiNote > 0) {
        synthManager.voice()->setInternalParameterValue(
            "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 432.f);
        synthManager.triggerOn(midiNote);
      }
    }
    return true;
  }

  // Whenever a key is released this function is called
  bool onKeyUp(Keyboard const &k) override {
    int midiNote = asciiToMIDI(k.key());
    if (midiNote > 0) {
      synthManager.triggerOff(midiNote);
    }
    return true;
  }

  void onExit() override { imguiShutdown(); }

  //From Professor Conrad's Frere Jacques Demo:
  void playNote(float freq, float time, float duration, float amp = .2, float attack = 0.6, float decay = 0.3)
  {
    auto *voice = synthManager.synth().getVoice<SquareWave>();
    // amp, freq, attack, release, pan
    vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playAddSyn(float freq, float time, float duration, float amp = .8, float attack = 0.8, float decay = 0.01)
  {
    auto *voice = synthManager.synth().getVoice<SineEnv>();
    // amp, freq, attack, release, pan
    vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playBass(float freq, float time, float duration, float amp = .9, float attack = 0.9, float decay = 0.001)
  {
    auto *voice = synthManager.synth().getVoice<SquareWave>();
    // amp, freq, attack, release, pan
    vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  //From Mitchell's code again:
  void playKick(float freq, float time, float duration = 0.5, float amp = 0.7, float attack = 0.9, float decay = 0.1)
  {
      auto *voice = synthManager.synth().getVoice<Kick>();
      // amp, freq, attack, release, pan
      vector<VariantValue> params = vector<VariantValue>({amp, freq, 0.01, 0.1, 0.0});
      voice->setTriggerParams(params);
      synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }
  void playSnare(float time, float duration = 0.3)
  {
      auto *voice = synthManager.synth().getVoice<Hihat>();
      // amp, freq, attack, release, pan
      synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  // **** ONTARIO REMAKE ****

  // HELPERS AND CONSTANTS
  // EACH OCTAVE DOUBLES FREQUENCY!
  const float C5 =  261.6 * 2;
  const float Csharp5 = 277.183 * 2;
  const float D5 = 293.7 * 2;
  const float E5 = 329.6 * 2;
  const float F5 = 349.2 * 2;
  const float Fsharp5 = 369.994 * 2;
  const float G5 = 392.0 * 2;
  const float Gsharp5 = 415.305 * 2;
  const float A5 = 440.0 * 2;
  const float Aflat5 = 466.2 * 2;
  const float B5 = 493.88 * 2;

  const float C4 =  261.6;
  const float Csharp4 = 277.183;
  const float D4 = 293.7;
  const float E4 = 329.6;
  const float F4 = 349.2;
  const float Fsharp4 = 369.994;
  const float G4 = 392.0;
  const float Gsharp4 = 415.305;
  const float A4 = 440.0;
  const float Aflat4 = 466.2;
  const float B4 = 493.88;

  const float C3 = C4 / 2;
  const float D3 = D4 / 2;
  const float E3 = E4 / 2;
  const float F3 = F4 / 2;
  const float Fsharp3 = Fsharp4 / 2;
  const float G3 = G4 / 2;
  const float A3 = A4 / 2;
  const float Aflat3 = Aflat4 / 2;
  const float B3 = B4 / 2;

  const float C2 = C3 / 2;
  const float D2 = D3 / 2;
  const float E2 = E3 / 2;
  const float F2 = F3 / 2;
  const float Fsharp2 = Fsharp3 / 2;
  const float G2 = G3 / 2;
  const float A2 = A3 / 2;
  const float Aflat2 = Aflat3 / 2;
  const float B2 = B3 / 2;

    const float A1 = A2 / 2;

  // Time
  const float bpm = 120;
  const float beat = 60 / bpm;
  const float measure = beat * 4;

  const float whole = half * 2;
  const float half = beat * 2;
  const float quarter = beat;
  const float eigth = quarter / 2;
  const float sixteenth = eigth / 2;

  // PARTS 
  void pianoIntro(int start) {
    playNote(Fsharp4, measure * 0 + beat * 0 + start, eigth);
    playNote(Fsharp3, measure * 0 + beat * 0.5 + start, half);
    playNote(A4, measure * 0 + beat * 1 + start, eigth);
    playNote(E5, measure * 0 + beat * 1.5 + start, eigth);
    playNote(Csharp5, measure * 0 + beat * 2 + start, eigth);
    playNote(Fsharp4, measure * 0 + beat * 2.5 + start, eigth);
    playNote(A4, measure * 0 + beat * 3 + start, eigth);
    playNote(Fsharp4, measure * 0 + beat * 3.5 + start, eigth);

    playNote(E4, measure * 1 + beat * 0 + start, eigth);
    playNote(E3, measure * 1 + beat * 0.5 + start, half);
    playNote(Gsharp4, measure * 1 + beat * 1 + start, eigth);
    playNote(E4, measure * 1 + beat * 1.5 + start, eigth);
    playNote(B4, measure * 1 + beat * 2 + start, eigth);
    playNote(A4, measure * 1 + beat * 2.5 + start, eigth);
    playNote(Gsharp4, measure * 1 + beat * 3 + start, eigth);
    playNote(A4, measure * 1 + beat * 3.5 + start, eigth);

    playNote(A2, measure * 2 + beat * 0 + start, half);
    playNote(A4, measure * 2 + beat * 0.5 + start, eigth);
    playNote(Csharp5, measure * 2 + beat * 1 + start, eigth);
    playNote(E5, measure * 2 + beat * 1.5 + start, eigth);
    playNote(B4, measure * 2 + beat * 2 + start, eigth);
    playNote(Csharp4, measure * 2 + beat * 2.5 + start, eigth);
    playNote(B4, measure * 2 + beat * 3 + start, eigth);
    playNote(A4, measure * 2 + beat * 3.5 + start, eigth);

    playNote(D4, measure * 3 + beat * 0 + start, eigth);
    playNote(D3, measure * 3 + beat * 0.5 + start, quarter);
    playNote(A4, measure * 3 + beat * 1 + start, eigth);
    playNote(Gsharp4, measure * 3 + beat * 1.5 + start, eigth);
    playNote(E4, measure * 3 + beat * 2 + start, eigth);
    playNote(E3, measure * 3 + beat * 2.5 + start, eigth);
    playNote(Gsharp4, measure * 3 + beat * 3 + start, eigth);
    playNote(A4, measure * 3 + beat * 3.5 + start, eigth);
  }

  void bass(int start) {
    playBass(Fsharp2, measure * 0 + beat * 0 + start, sixteenth);
    playBass(Fsharp2, measure * 0 + beat * 0.5 + start, sixteenth);
    playBass(Fsharp2, measure * 0 + beat * 1 + start, sixteenth);
    playBass(Fsharp2, measure * 0 + beat * 1.5 + start, sixteenth);
    playBass(Fsharp2, measure * 0 + beat * 2 + start, sixteenth);
    playBass(Fsharp2, measure * 0 + beat * 2.5 + start, sixteenth);
    playBass(Fsharp2, measure * 0 + beat * 3 + start, sixteenth);
    playBass(Fsharp2, measure * 0 + beat * 3.5 + start, sixteenth);

    playBass(E2, measure * 1 + beat * 0 + start, sixteenth);
    playBass(E2, measure * 1 + beat * 0.5 + start, sixteenth);
    playBass(E2, measure * 1 + beat * 1 + start, sixteenth);
    playBass(E2, measure * 1 + beat * 1.5 + start, sixteenth);
    playBass(E2, measure * 1 + beat * 2 + start, sixteenth);
    playBass(E2, measure * 1 + beat * 2.5 + start, sixteenth);
    playBass(E2, measure * 1 + beat * 3 + start, sixteenth);
    playBass(E2, measure * 1 + beat * 3.5 + start, sixteenth);

    playBass(A1, measure * 2 + beat * 0 + start, sixteenth);
    playBass(A1, measure * 2 + beat * 0.5 + start, sixteenth);
    playBass(A1, measure * 2 + beat * 1 + start, sixteenth);
    playBass(A1, measure * 2 + beat * 1.5 + start, sixteenth);
    playBass(A1, measure * 2 + beat * 2 + start, sixteenth);
    playBass(A1, measure * 2 + beat * 2.5 + start, sixteenth);
    playBass(A1, measure * 2 + beat * 3 + start, sixteenth);
    playBass(A1, measure * 2 + beat * 3.5 + start, sixteenth);

    playBass(D2, measure * 3 + beat * 0 + start, sixteenth);
    playBass(D2, measure * 3 + beat * 0.5 + start, sixteenth);
    playBass(D2, measure * 3 + beat * 1 + start, sixteenth);
    playBass(D2, measure * 3 + beat * 1.5 + start, sixteenth);
    playBass(E2, measure * 3 + beat * 2 + start, sixteenth);
    playBass(E2, measure * 3 + beat * 2.5 + start, sixteenth);
    playBass(E2, measure * 3 + beat * 3 + start, sixteenth);
    playBass(E2, measure * 3 + beat * 3.5 + start, sixteenth);
  }

  void kickRhythm(int start) {
    playKick(150, start);
    playKick(150, start + 0.5);
    playKick(150, start + 1);
    playKick(150, start + 1.5);
    playKick(150, start + 2);
    playKick(150, start + 2.5);
    playKick(150, start + 3);
    playKick(150, start + 3.5);
  }

  void synthChorus(int start) {
    // synth chorus melody
    // measure 1
    playAddSyn(Fsharp2, measure * 0 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(Fsharp3, measure * 0 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(Fsharp4, measure * 0 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(A4, measure * 0 + beat * 0 + start, eigth + sixteenth);

    playAddSyn(Fsharp2, measure * 0 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(Fsharp3, measure * 0 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(Fsharp4, measure * 0 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(Gsharp4, measure * 0 + beat * 0.75 + start, eigth + sixteenth);

    playAddSyn(Fsharp2, measure * 0 + beat * 1.5 + start, quarter);
    playAddSyn(Fsharp3, measure * 0 + beat * 1.5 + start, quarter);
    playAddSyn(Fsharp4, measure * 0 + beat * 1.5 + start, quarter);
    playAddSyn(A4, measure * 0 + beat * 1.5 + start, quarter);

    playAddSyn(Fsharp2, measure * 0 + beat * 2.5 + start, eigth);
    playAddSyn(Fsharp3, measure * 0 + beat * 2.5 + start, eigth);
    playAddSyn(Fsharp4, measure * 0 + beat * 2.5 + start, eigth);
    playAddSyn(Csharp5, measure * 0 + beat * 2.5 + start, eigth);

    playAddSyn(Fsharp2, measure * 0 + beat * 3 + start, eigth);
    playAddSyn(Fsharp3, measure * 0 + beat * 3 + start, eigth);
    playAddSyn(Fsharp4, measure * 0 + beat * 3 + start, eigth);
    playAddSyn(A4, measure * 0 + beat * 3 + start, eigth);

    playAddSyn(Fsharp2, measure * 0 + beat * 3.5 + start, eigth);
    playAddSyn(Fsharp3, measure * 0 + beat * 3.5 + start, eigth);
    playAddSyn(Fsharp4, measure * 0 + beat * 3.5 + start, eigth);
    playAddSyn(Gsharp4, measure * 0 + beat * 3.5 + start, eigth);

    // measure 2
    playAddSyn(A2, measure * 1 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(A3, measure * 1 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(E4, measure * 1 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(A4, measure * 1 + beat * 0 + start, eigth + sixteenth);

    playAddSyn(A2, measure * 1 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(A3, measure * 1 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(E4, measure * 1 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(Gsharp4, measure * 1 + beat * 0.75 + start, eigth + sixteenth);

    playAddSyn(A2, measure * 1 + beat * 1.5 + start, quarter);
    playAddSyn(A3, measure * 1 + beat * 1.5 + start, quarter);
    playAddSyn(E4, measure * 1 + beat * 1.5 + start, quarter);
    playAddSyn(A4, measure * 1 + beat * 1.5 + start, quarter);

    playAddSyn(A2, measure * 1 + beat * 2.5 + start, eigth);
    playAddSyn(A3, measure * 1 + beat * 2.5 + start, eigth);
    playAddSyn(Gsharp4, measure * 1 + beat * 2.5 + start, eigth);

    playAddSyn(A2, measure * 1 + beat * 3 + start, eigth);
    playAddSyn(A3, measure * 1 + beat * 3 + start, eigth);
    playAddSyn(A4, measure * 1 + beat * 3 + start, eigth);

    playAddSyn(A2, measure * 1 + beat * 3.5 + start, eigth);
    playAddSyn(A3, measure * 1 + beat * 3.5 + start, eigth);
    playAddSyn(E4, measure * 1 + beat * 3.5 + start, eigth);

    // measure 3
    playAddSyn(E2, measure * 2 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(E3, measure * 2 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(E4, measure * 2 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(A4, measure * 2 + beat * 0 + start, eigth + sixteenth);

    playAddSyn(E2, measure * 2 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(E3, measure * 2 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(E4, measure * 2 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(Gsharp4, measure * 2 + beat * 0.75 + start, eigth + sixteenth);

    playAddSyn(E2, measure * 2 + beat * 1.5 + start, quarter);
    playAddSyn(E3, measure * 2 + beat * 1.5 + start, quarter);
    playAddSyn(E4, measure * 2 + beat * 1.5 + start, quarter);
    playAddSyn(A4, measure * 2 + beat * 1.5 + start, quarter);

    playAddSyn(E2, measure * 2 + beat * 2.5 + start, eigth);
    playAddSyn(E3, measure * 2 + beat * 2.5 + start, eigth);
    playAddSyn(Csharp5, measure * 2 + beat * 2.5 + start, eigth);

    playAddSyn(E2, measure * 2 + beat * 3 + start, eigth);
    playAddSyn(E3, measure * 2 + beat * 3 + start, eigth);
    playAddSyn(A4, measure * 2 + beat * 3 + start, eigth);

    playAddSyn(E2, measure * 2 + beat * 3.5 + start, eigth);
    playAddSyn(E3, measure * 2 + beat * 3.5 + start, eigth);
    playAddSyn(Gsharp4, measure * 2 + beat * 3.5 + start, eigth);


    // measure 4
    playAddSyn(D2, measure * 3 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(D3, measure * 3 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(D4, measure * 3 + beat * 0 + start, eigth + sixteenth);
    playAddSyn(A4, measure * 3 + beat * 0 + start, eigth + sixteenth);

    playAddSyn(D2, measure * 3 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(D3, measure * 3 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(D4, measure * 3 + beat * 0.75 + start, eigth + sixteenth);
    playAddSyn(Gsharp4, measure * 3 + beat * 0.75 + start, eigth + sixteenth);

    playAddSyn(D2, measure * 3 + beat * 1.5 + start, quarter);
    playAddSyn(D3, measure * 3 + beat * 1.5 + start, quarter);
    playAddSyn(D4, measure * 3 + beat * 1.5 + start, quarter);
    playAddSyn(A4, measure * 3 + beat * 1.5 + start, quarter);

    playAddSyn(D2, measure * 3 + beat * 2.5 + start, eigth);
    playAddSyn(D3, measure * 3 + beat * 2.5 + start, eigth);
    playAddSyn(Gsharp4, measure * 3 + beat * 2.5 + start, eigth);

    playAddSyn(D2, measure * 3 + beat * 3 + start, eigth);
    playAddSyn(D3, measure * 3 + beat * 3 + start, eigth);
    playAddSyn(A4, measure * 3 + beat * 3 + start, eigth);

    playAddSyn(D2, measure * 3 + beat * 3.5 + start, eigth);
    playAddSyn(D3, measure * 3 + beat * 3.5 + start, eigth);
    playAddSyn(Gsharp4, measure * 3 + beat * 3.5 + start, eigth);
  }

  void synthCounter(int bpm, float vol) {
    // synth chorus countermelody

  }

  void riserSnare(int start) {
    // riser snare from piano intro to synth chorus
    // 2 bars eigth notes, 1 bar 16th notes, 1 bar 32nd notes

    playSnare(measure * 0 + beat * 0 + start);
    playSnare(measure * 0 + beat * 0.5 + start);
    playSnare(measure * 0 + beat * 1 + start);
    playSnare(measure * 0 + beat * 1.5 + start);
    playSnare(measure * 0 + beat * 2 + start);
    playSnare(measure * 0 + beat * 2.5 + start);
    playSnare(measure * 0 + beat * 3 + start);
    playSnare(measure * 0 + beat * 3.5 + start);

    playSnare(measure * 1 + beat * 0 + start);
    playSnare(measure * 1 + beat * 0.5 + start);
    playSnare(measure * 1 + beat * 1 + start);
    playSnare(measure * 1 + beat * 1.5 + start);
    playSnare(measure * 1 + beat * 2 + start);
    playSnare(measure * 1 + beat * 2.5 + start);
    playSnare(measure * 1 + beat * 3 + start);
    playSnare(measure * 1 + beat * 3.5 + start);

    playSnare(measure * 2 + beat * 0 + start);
    playSnare(measure * 2 + beat * 0.25 + start);
    playSnare(measure * 2 + beat * 0.5 + start);
    playSnare(measure * 2 + beat * 0.75 + start);
    playSnare(measure * 2 + beat * 1 + start);
    playSnare(measure * 2 + beat * 1.25 + start);
    playSnare(measure * 2 + beat * 1.5 + start);
    playSnare(measure * 2 + beat * 1.75 + start);
    playSnare(measure * 2 + beat * 2 + start);
    playSnare(measure * 2 + beat * 2.25 + start);
    playSnare(measure * 2 + beat * 2.5 + start);
    playSnare(measure * 2 + beat * 2.75 + start);
    playSnare(measure * 2 + beat * 3 + start);
    playSnare(measure * 2 + beat * 3.25 + start);
    playSnare(measure * 2 + beat * 3.5 + start);
    playSnare(measure * 2 + beat * 3.75 + start);

    playSnare(measure * 3 + beat * 0 + start);
    playSnare(measure * 3 + beat * 0.125 + start);
    playSnare(measure * 3 + beat * 0.25 + start);
    playSnare(measure * 3 + beat * 0.375 + start);
    playSnare(measure * 3 + beat * 0.5 + start);
    playSnare(measure * 3 + beat * 0.675 + start);
    playSnare(measure * 3 + beat * 0.75 + start);
    playSnare(measure * 3 + beat * 0.875 + start);
    playSnare(measure * 3 + beat * 1 + start);
    playSnare(measure * 3 + beat * 1.125 + start);
    playSnare(measure * 3 + beat * 1.25 + start);
    playSnare(measure * 3 + beat * 1.375 + start);
    playSnare(measure * 3 + beat * 1.5 + start);
    playSnare(measure * 3 + beat * 1.675 + start);
    playSnare(measure * 3 + beat * 1.75 + start);
    playSnare(measure * 3 + beat * 1.875 + start);
    playSnare(measure * 3 + beat * 2 + start);
    playSnare(measure * 3 + beat * 2.125 + start);
    playSnare(measure * 3 + beat * 2.25 + start);
    playSnare(measure * 3 + beat * 2.375 + start);
    playSnare(measure * 3 + beat * 2.5 + start);
    playSnare(measure * 3 + beat * 2.675 + start);
    playSnare(measure * 3 + beat * 2.75 + start);
    playSnare(measure * 3 + beat * 2.875 + start);
    playSnare(measure * 3 + beat * 3 + start);
    playSnare(measure * 3 + beat * 3.125 + start);
    playSnare(measure * 3 + beat * 3.25 + start);
    playSnare(measure * 3 + beat * 3.375 + start);
    playSnare(measure * 3 + beat * 3.5 + start);
    playSnare(measure * 3 + beat * 3.675 + start);
    playSnare(measure * 3 + beat * 3.75 + start);
    playSnare(measure * 3 + beat * 3.875 + start);
  }

  void rhythmSnare(int start) {
    playSnare(measure * 0 + beat * 0 + start);
    playSnare(measure * 0 + beat * 0.5 + start);
    playSnare(measure * 0 + beat * 1 + start);
    playSnare(measure * 0 + beat * 1.5 + start);
    playSnare(measure * 0 + beat * 2 + start);
    playSnare(measure * 0 + beat * 2.5 + start);
    playSnare(measure * 0 + beat * 3 + start);
    playSnare(measure * 0 + beat * 3.5 + start);

  }

  void playTune(){ //one measure of drums, then the tune!
    for(int i=0; i<24; i++) {
      if (i % 8 == 0) {
        pianoIntro(i);
      }
    }
    for (int i=8; i<40; i++) {
      if(i % 4 == 0) {
        kickRhythm(i);
      }
    }
    for(int i=16; i<24; i++) {
      if(i % 8 == 0) {
        riserSnare(i);
      }
    }
    for(int i=24; i<40; i++) {
      if(i % 8 == 0) {
        synthChorus(i);
      }
    }
    for(int i=24; i<40; i++) {
      rhythmSnare(i);
    }
    for(int i=40; i<48; i++) {
      if (i % 8 == 0) {
        pianoIntro(i);
      }
    }
  }

};

int main() {
  // Create app instance
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();
  return 0;
}