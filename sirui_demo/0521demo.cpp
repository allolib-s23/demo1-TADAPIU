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

class SineEnv : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc1;
  gam::Sine<> mOsc3;
  gam::Saw<> mSaw1;
  gam::Saw<> mSaw2;
  gam::Saw<> mSaw3;
  gam::Env<3> mAmpEnv;

  gam::EnvFollow<> mEnvFollow;
  Mesh mMesh;
  double time = 1;
  double time2 = 0;

  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override
  {
    // Intialize envelope
    mAmpEnv.curve(0); // make segments lines
    mAmpEnv.levels(0, 1, 0.8, 0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    // This is a quick way to create parameters for the voice. Trigger
    // parameters are meant to be set only when the voice starts, i.e. they
    // are expected to be constant within a voice instance. (You can actually
    // change them while you are prototyping, but their changes will only be
    // stored and aplied when a note is triggered.)

    //addCone(mMesh);
    addDodecahedron(mMesh,0.4);
    addCircle(mMesh, 0.7);

    createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.2, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.2, 0.1, 10.0);
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
    mOsc1.freq(getInternalParameterValue("frequency"));
    mOsc3.freq(3*getInternalParameterValue("frequency"));
    mSaw1.freq(getInternalParameterValue("frequency"));
    mSaw3.freq(3*getInternalParameterValue("frequency"));
    mSaw2.freq(2*getInternalParameterValue("frequency"));
  

    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io())
    {
      
      float s1 =  // mSaw1() * (1.0) * mAmpEnv() * getInternalParameterValue("amplitude")
              // + mSaw3() * (1.0/6.0) * mAmpEnv() * getInternalParameterValue("amplitude")
              mSaw2() * (1.0/2.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc1() * (1.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc3() * (1.0/3.0) * mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done()){
      free();
    }
  }

  // The graphics processing function
  void onProcess(Graphics &g) override
  {
    // empty if there are no graphics to draw
    // Get the paramter values on every video frame, to apply changes to the
    // current instance
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    time += 0.02;
    // Now draw
    g.pushMatrix();
    // Move x according to frequency, y according to amplitude
    g.translate(frequency / 300 * (time) - 4, frequency / 100 * cos(time) +2, -14);
    // Scale in the x and y directions according to amplitude
    g.scale(frequency/1000, frequency/700, 1);
    // Set the color. Red and Blue according to sound amplitude and Green
    // according to frequency. Alpha fixed to 0.4
    //g.color(frequency / 1000, amplitude / 1000,  mEnvFollow.value(), 0.4);
    g.color(HSV(frequency/100*sin(0.05*time)));
    //g.numLight(10);
    g.draw(mMesh);
    g.popMatrix();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }

  void onTriggerOff() override { mAmpEnv.release(); }
};

class SquareWave : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  gam::Sine<> mOsc1;
  gam::Sine<> mOsc3;
  gam::Sine<> mOsc5;
  gam::Sine<> mOsc7;
  gam::Sine<> mOsc9;
  gam::Env<3> mAmpEnv;

  gam::EnvFollow<> mEnvFollow;
  Mesh mMesh;
  double time = 3.5;
  double time2 = 0;

  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override
  {
    // Intialize envelope
    mAmpEnv.curve(0); // make segments lines
    mAmpEnv.levels(0, 1, 0.8, 0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    // This is a quick way to create parameters for the voice. Trigger
    // parameters are meant to be set only when the voice starts, i.e. they
    // are expected to be constant within a voice instance. (You can actually
    // change them while you are prototyping, but their changes will only be
    // stored and aplied when a note is triggered.)

    addSphere(mMesh, 0.5);
    //addAnnulus(mMesh, 1.2,1.5);
    addWireBox(mMesh, 1.5);

    createInternalTriggerParameter("amplitude", 0.3, 0.0, 1.0);
    createInternalTriggerParameter("frequency", 60, 20, 5000);
    createInternalTriggerParameter("attackTime", 0.2, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.2, 0.1, 10.0);
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
    mOsc1.freq(getInternalParameterValue("frequency"));
    mOsc3.freq(3*getInternalParameterValue("frequency"));
    mOsc5.freq(5*getInternalParameterValue("frequency"));
    mOsc7.freq(7*getInternalParameterValue("frequency"));
    mOsc9.freq(9*getInternalParameterValue("frequency"));
  

    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io())
    {
      
      float s1 =  mOsc1() * (1.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc3() * (1.0/3.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc5() * (1.0/5.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc7() * (1.0/7.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc9() * (1.0/9.0) * mAmpEnv() * getInternalParameterValue("amplitude");
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done()){
      free();
    }
  }

  // The graphics processing function
  void onProcess(Graphics &g) override
  {
    // empty if there are no graphics to draw
    // Get the paramter values on every video frame, to apply changes to the
    // current instance
    float frequency = getInternalParameterValue("frequency");
    float amplitude = getInternalParameterValue("amplitude");
    time += 0.03;
    time2 -= 0.02;
    // Now draw
    g.pushMatrix();
    // Move x according to frequency, y according to amplitude
    g.translate(frequency / 360 * (0.1*time) + 1.5, frequency / 180 * sin(0.05*time2) + 0.5, -10);
    // Scale in the x and y directions according to amplitude
    g.scale(frequency/1200, frequency/850, 1.2);
    // Set the color. Red and Blue according to sound amplitude and Green
    // according to frequency. Alpha fixed to 0.4
    //g.color(frequency / 1000, amplitude / 1000,  mEnvFollow.value(), 0.4);
    g.color(HSV(frequency/200));
    //g.numLight(10);
    g.draw(mMesh);
    g.popMatrix();
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }

  void onTriggerOff() override { mAmpEnv.release(); }
};

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
    gam::STFT stft = gam::STFT(FFT_SIZE, FFT_SIZE / 4, 0, gam::BLACKMAN_HARRIS, gam::MAG_FREQ);
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
        a += 0.6;
        b += 0.28;
        timepose -= 0.09;

        mSpectrogram.reset();
        // mSpectrogram.primitive(Mesh::LINE_STRIP);

        for (int i = 0; i < FFT_SIZE / 2; i++)
        {
            // mSpectrogram.color(HSV(spectrum[i] * 1000 + al::rnd::uniform()));
            mSpectrogram.color(HSV(frequency/800));
            mSpectrogram.vertex(3*i, 5*spectrum[i], 0.0);
        }
        g.meshColor(); // Use the color in the mesh
        g.pushMatrix();
        g.translate(0, -0.5, -10);
        g.rotate(a, Vec3f(0, 1, 0));
        g.rotate(b, Vec3f(1));
        g.scale(30.0 / FFT_SIZE, 500, 1.0);
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
      float s1 = 0.7*mBurst();
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

// We make an app.
class MyApp : public App {
public:

// GUI manager for SineEnv voices
  // The name provided determines the name of the directory
  // where the presets and sequences are stored
  SynthGUIManager<SineEnv> synthManager{"SineEnv"};

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
    playSong();
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
            "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * 440.f);
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


  void playSineEnv(float freq, float time, float duration, float amp = .07, float attack = 0.25, float decay = 0.2)
  {
    auto *voice = synthManager.synth().getVoice<SineEnv>();
    // amp, freq, attack, release, pan
    vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playSquareWave(float freq, float time, float duration, float amp = .07, float attack = 0.1, float decay = 0.2)
  {
    auto *voice = synthManager.synth().getVoice<SquareWave>();
    // amp, freq, attack, release, pan
    vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playPluckString(float freq, float time, float duration, float amp = 0.03)
  {
    
    auto *voice = synthManager.synth().getVoice<PluckedString>();
    //vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
    //voice->setTriggerParams(params);

    voice->setInternalParameterValue("frequency", freq);
    voice->setInternalParameterValue("amplitude", amp);

    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playHihat(float time, float duration = 0.3)
  {
      auto *voice = synthManager.synth().getVoice<Hihat>();
      // amp, freq, attack, release, pan
      synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  // NOTES AND CORRESPONDING FREQUENCIES
  const float Csharp5 = 554.37;
  const float D5 = 587.33;
  const float E5 = 659.25;
  const float Fsharp5 = 739.99;
  const float G5 = 783.99;
  const float A5 = 880.0;
  const float B5 = 987.77;

  const float Csharp4 = 277.18;
  const float D4 = 293.66;
  const float E4 = 329.63;
  const float Fsharp4 = 369.99;
  const float G4 = 392.0;
  const float A4 = 440.0;
  const float B4 = 493.88;

  const float Csharp3 = Csharp4 / 2;
  const float D3 = D4 / 2;
  const float E3 = E4 / 2;
  const float Fsharp3 = Fsharp4 / 2;
  const float G3 = G4 / 2;
  const float A3 = A4 / 2;
  const float B3 = B4 / 2;

  const float Csharp2 = Csharp3 / 2;
  const float D2 = D3 / 2;
  const float E2 = E3 / 2;
  const float Fsharp2 = Fsharp3 / 2;
  const float G2 = G3 / 2;
  const float A2 = A3 / 2;
  const float B2 = B3 / 2;

  const float G1 = G2 / 2;
  const float A1 = A2 / 2;
  const float B1 = B2 / 2;

  const float bpm = 135;
  const float beat = 60 / bpm;
  const float measure = beat * 4;

  const float whole = measure;
  const float half = beat * 2;
  const float quarter = beat;
  const float eigth = quarter / 2;
  const float sixteenth = eigth / 2;

  void part1(float time){
    playSineEnv(Fsharp4, measure * 0 + beat * 0 + time, whole,.1);
    playSineEnv(D3, measure * 0 + beat * 0 + time, whole,.1);
    playSineEnv(E4, measure * 1 + beat * 0 + time, whole,.09);
    playSineEnv(A2, measure * 1 + beat * 0 + time, whole,.09);
    playSineEnv(D4, measure * 2 + beat * 0 + time, whole,.08);
    playSineEnv(B2, measure * 2 + beat * 0 + time, whole,.08);
    playSineEnv(Csharp4, measure * 3 + beat * 0 + time, whole,.07);
    playSineEnv(Fsharp2, measure * 3 + beat * 0 + time, whole,.07);
    playSineEnv(B3, measure * 4 + beat * 0 + time, whole,.06);
    playSineEnv(G2, measure * 4 + beat * 0 + time, whole,.06);
    playSineEnv(A3, measure * 5 + beat * 0 + time, whole,.07);
    playSineEnv(D2, measure * 5 + beat * 0 + time, whole,.07);
    playSineEnv(B3, measure * 6 + beat * 0 + time, whole,.08);
    playSineEnv(G2, measure * 6 + beat * 0 + time, whole,.08);
    playSineEnv(Csharp4, measure * 7 + beat * 0 + time, whole,.09);
    playSineEnv(A2, measure * 7 + beat * 0 + time, whole,.09);
  }

  void part2(float time){
    playSineEnv(Fsharp5, measure * 0 + beat * 0 + time, whole,.05);
    playSineEnv(D5, measure * 0 + beat * 0 + time, whole,.05);
    playSineEnv(D4, measure * 0 + beat * 0 + time, whole,.05);
    playSineEnv(D3, measure * 0 + beat * 0 + time, whole,.05);
    playSineEnv(Csharp5, measure * 1 + beat * 0 + time, whole,.05);
    playSineEnv(E5, measure * 1 + beat * 0 + time, whole,.045);
    playSineEnv(A3, measure * 1 + beat * 0 + time, whole,.045);
    playSineEnv(A2, measure * 1 + beat * 0 + time, whole,.045);
    playSineEnv(G4, measure * 1 + beat * 2 + time, half,.045);
    playSineEnv(D5, measure * 2 + beat * 0 + time, whole,.04);
    playSineEnv(B4, measure * 2 + beat * 0 + time, whole,.04);
    playSineEnv(B3, measure * 2 + beat * 0 + time, whole,.04);
    playSineEnv(B2, measure * 2 + beat * 0 + time, whole,.04);
    playSineEnv(Csharp5, measure * 3 + beat * 0 + time, whole,.045);
    playSineEnv(A4, measure * 3 + beat * 0 + time, whole,.045);
    playSineEnv(Fsharp3, measure * 3 + beat * 0 + time, whole,.045);
    playSineEnv(Fsharp2, measure * 3 + beat * 0 + time, whole,.045);
    playSineEnv(E4, measure * 3 + beat * 2 + time, half,.045);
    playSineEnv(B4, measure * 4 + beat * 0 + time, whole,.05);
    playSineEnv(D4, measure * 4 + beat * 0 + time, whole,.05);
    playSineEnv(G3, measure * 4 + beat * 0 + time, whole,.05);
    playSineEnv(G2, measure * 4 + beat * 0 + time, whole,.05);
    playSineEnv(A4, measure * 5 + beat * 0 + time, half,.045);
    playSineEnv(Fsharp4, measure * 5 + beat * 0 + time, whole,.045);
    playSineEnv(D3, measure * 5 + beat * 0 + time, whole,.045);
    playSineEnv(D2, measure * 5 + beat * 0 + time, whole,.045);
    playSineEnv(A4, measure * 5 + beat * 2 + time, half,.045);
    playSineEnv(B4, measure * 6 + beat * 0 + time, whole,.04);
    playSineEnv(G4, measure * 6 + beat * 0 + time, whole,.04);
    playSineEnv(G3, measure * 6 + beat * 0 + time, whole,.04);
    playSineEnv(G2, measure * 6 + beat * 0 + time, whole,.04);
    playSineEnv(Csharp5, measure * 7 + beat * 0 + time, whole,.045);
    playSineEnv(A4, measure * 7 + beat * 0 + time, whole,.045);
    playSineEnv(A3, measure * 7 + beat * 0 + time, whole,.045);
    playSineEnv(A2, measure * 7 + beat * 0 + time, whole,.045);
    playSineEnv(G4, measure * 7 + beat * 2 + time, half,.045);

  }

  void part3_left(float time){
    playSineEnv(Fsharp5,  measure * 0 + beat * 0 + time, whole,.07);
    playSineEnv(D3,       measure * 0 + beat * 0 + time, whole,.07);
    // playSineEnv(A3,       measure * 0 + beat * 1 + time, quarter);
    // playSineEnv(D4,       measure * 0 + beat * 2 + time, half);
    playSineEnv(E5,       measure * 1 + beat * 0 + time, whole,.065);
    playSineEnv(A2,       measure * 1 + beat * 0 + time, whole,.065);
    // playSineEnv(E3,       measure * 1 + beat * 1 + time, quarter);
    // playSineEnv(G3,       measure * 1 + beat * 2 + time, half);
    playSineEnv(D5,       measure * 2 + beat * 0 + time, whole,.06);
    playSineEnv(B2,       measure * 2 + beat * 0 + time, whole,.06);
    // playSineEnv(Fsharp3,  measure * 2 + beat * 1 + time, quarter);
    // playSineEnv(B3,       measure * 2 + beat * 2 + time, half);
    playSineEnv(Csharp5,  measure * 3 + beat * 0 + time, whole,.065);
    playSineEnv(Fsharp2,  measure * 3 + beat * 0 + time, whole,.065);
    // playSineEnv(Csharp3,  measure * 3 + beat * 1 + time, quarter);
    // playSineEnv(Fsharp3,  measure * 3 + beat * 2 + time, half);

    playSineEnv(B4,       measure * 4 + beat * 0 + time, whole,0.05);
    playSineEnv(G4,       measure * 4 + beat * 0 + time, whole,0.05);
    playSineEnv(G2,       measure * 4 + beat * 0 + time, whole,0.05);
    // playSineEnv(D3,       measure * 4 + beat * 1 + time, quarter);
    // playSineEnv(G3,       measure * 4 + beat * 2 + time, quarter);
    // playSineEnv(D3,       measure * 4 + beat * 3 + time, quarter);

    playSineEnv(A4,       measure * 5 + beat * 0 + time, whole,0.05);
    playSineEnv(Fsharp4,  measure * 5 + beat * 0 + time, whole,0.05);
    playSineEnv(D2,       measure * 5 + beat * 0 + time, whole,0.05);
    // playSineEnv(A2,       measure * 5 + beat * 1 + time, quarter);
    // playSineEnv(D3,       measure * 5 + beat * 2 + time, quarter);
    // playSineEnv(A2,       measure * 5 + beat * 3 + time, quarter);

    playSineEnv(B4,       measure * 6 + beat * 0 + time, whole,0.045);
    playSineEnv(G4,       measure * 6 + beat * 0 + time, whole,0.045);
    playSineEnv(G2,       measure * 6 + beat * 0 + time, whole,0.045);
    // playSineEnv(D3,       measure * 6 + beat * 1 + time, quarter);
    // playSineEnv(G3,       measure * 6 + beat * 2 + time, quarter);
    // playSineEnv(D3,       measure * 6 + beat * 3 + time, quarter);

    playSineEnv(Csharp5,  measure * 7 + beat * 0 + time, whole,0.05);
    playSineEnv(A4,       measure * 7 + beat * 0 + time, whole,0.05);
    playSineEnv(A2,       measure * 7 + beat * 0 + time, whole,0.05);
    // playSineEnv(E3,       measure * 7 + beat * 1 + time, quarter);
    // playSineEnv(A3,       measure * 7 + beat * 2 + time, quarter);
    // playSineEnv(E3,       measure * 7 + beat * 3 + time, quarter);
  }

  void part3_right(float time){
    playSquareWave(A3,       measure * 0 + beat * 1 + time, quarter,.05);
    playSquareWave(D4,       measure * 0 + beat * 2 + time, half,.07);
    playSquareWave(E3,       measure * 1 + beat * 1 + time, quarter,.05);
    playSquareWave(G3,       measure * 1 + beat * 2 + time, half,.07);
    playSquareWave(Fsharp3,  measure * 2 + beat * 1 + time, quarter,.05);
    playSquareWave(B3,       measure * 2 + beat * 2 + time, half,.07);
    playSquareWave(Csharp3,  measure * 3 + beat * 1 + time, quarter,.05);
    playSquareWave(Fsharp3,  measure * 3 + beat * 2 + time, half,.04);

    playSquareWave(D3,       measure * 4 + beat * 1 + time, quarter,.03);
    playSquareWave(G3,       measure * 4 + beat * 2 + time, quarter,.03);
    playSquareWave(D3,       measure * 4 + beat * 3 + time, quarter,.03);

    playSquareWave(A2,       measure * 5 + beat * 1 + time, quarter,.04);
    playSquareWave(D3,       measure * 5 + beat * 2 + time, quarter,.04);
    playSquareWave(A2,       measure * 5 + beat * 3 + time, quarter,.04);

    playSquareWave(D3,       measure * 6 + beat * 1 + time, quarter,.03);
    playSquareWave(G3,       measure * 6 + beat * 2 + time, quarter,.03);
    playSquareWave(D3,       measure * 6 + beat * 3 + time, quarter,.03);

    playSquareWave(E3,       measure * 7 + beat * 1 + time, quarter,.04);
    playSquareWave(A3,       measure * 7 + beat * 2 + time, quarter,.045);
    playSquareWave(E3,       measure * 7 + beat * 3 + time, quarter,.05);
  }

  void part4_left_hand(float time){
    playSineEnv(D3,       measure * 0 + beat * 0 + time, whole,.07);
    playSineEnv(A3,       measure * 0 + beat * 1 + time, quarter,.065);
    playSineEnv(D4,       measure * 0 + beat * 2 + time, quarter,.06);
    playSineEnv(A3,       measure * 0 + beat * 3 + time, quarter,.055);
   
    playSineEnv(A2,       measure * 1 + beat * 0 + time, whole,.055);
    playSineEnv(E3,       measure * 1 + beat * 1 + time, quarter,.06);
    playSineEnv(A3,       measure * 1 + beat * 2 + time, quarter,.065);
    playSineEnv(E3,       measure * 1 + beat * 3 + time, quarter,.07);
    
    playSineEnv(B2,       measure * 2 + beat * 0 + time, whole);
    playSineEnv(Fsharp3,  measure * 2 + beat * 1 + time, quarter,.065);
    playSineEnv(B3,       measure * 2 + beat * 2 + time, quarter,.06);
    playSineEnv(Fsharp3,  measure * 2 + beat * 3 + time, quarter,.055);
  
    playSineEnv(Fsharp2,  measure * 3 + beat * 0 + time, whole,.055);
    playSineEnv(Csharp3,  measure * 3 + beat * 1 + time, quarter,.06);
    playSineEnv(Fsharp3,  measure * 3 + beat * 2 + time, quarter,.065);
    playSineEnv(Csharp3,  measure * 3 + beat * 3 + time, quarter);
    
    playSineEnv(G2,       measure * 4 + beat * 0 + time, whole);
    playSineEnv(D3,       measure * 4 + beat * 1 + time, quarter,.065);
    playSineEnv(G3,       measure * 4 + beat * 2 + time, quarter,.06);
    playSineEnv(D3,       measure * 4 + beat * 3 + time, quarter,.055);
   
    playSineEnv(D2,       measure * 5 + beat * 0 + time, whole,.055);
    playSineEnv(A2,       measure * 5 + beat * 1 + time, quarter,.06);
    playSineEnv(D3,       measure * 5 + beat * 2 + time, quarter,.065);
    playSineEnv(A2,       measure * 5 + beat * 3 + time, quarter);
    
    playSineEnv(G2,       measure * 6 + beat * 0 + time, whole);
    playSineEnv(D3,       measure * 6 + beat * 1 + time, quarter,.065);
    playSineEnv(G3,       measure * 6 + beat * 2 + time, quarter,.06);
    playSineEnv(D3,       measure * 6 + beat * 3 + time, quarter,.055);
  
    playSineEnv(A2,       measure * 7 + beat * 0 + time, whole,.055);
    playSineEnv(E3,       measure * 7 + beat * 1 + time, quarter,.06);
    playSineEnv(A3,       measure * 7 + beat * 2 + time, quarter,.065);
    playSineEnv(E3,       measure * 7 + beat * 3 + time, quarter);

  }

  void part4_right_hand(float time){
    playSquareWave(D5,       measure * 0 + beat * 0 + time, quarter);
    playSquareWave(Csharp5,  measure * 0 + beat * 1 + time, quarter, .05);
    playSquareWave(D5,       measure * 0 + beat * 2 + time, half,.05);
    playSquareWave(Csharp5,  measure * 1 + beat * 0 + time, whole,.06);
    playSquareWave(D5,       measure * 2 + beat * 0 + time, half+quarter,.05);
    playSquareWave(Fsharp5,  measure * 2 + beat * 3 + time, quarter,.06);
    playSquareWave(A5,       measure * 3 + beat * 0 + time, half+quarter);
    playSquareWave(B5,       measure * 3 + beat * 3 + time, quarter);
    playSquareWave(G5,       measure * 4 + beat * 0 + time, quarter,.06);
    playSquareWave(Fsharp5,  measure * 4 + beat * 1 + time, quarter,.05);
    playSquareWave(E5,       measure * 4 + beat * 2 + time, quarter+eigth);
    playSquareWave(G5,       measure * 4 + beat * 3.5 + time, eigth);
    playSquareWave(Fsharp5,  measure * 5 + beat * 0 + time, quarter);
    playSquareWave(E5,       measure * 5 + beat * 1 + time, quarter);
    playSquareWave(D5,       measure * 5 + beat * 2 + time, quarter+eigth);
    playSquareWave(Csharp5,  measure * 5 + beat * 3.5 + time, eigth,.05);
    playSquareWave(B4,       measure * 6 + beat * 0 + time, quarter+eigth);
    playSquareWave(A4,       measure * 6 + beat * 1.5 + time, eigth,.05);
    playSquareWave(G4,       measure * 6 + beat * 2 + time, quarter);
    playSquareWave(Fsharp4,  measure * 6 + beat * 3 + time, quarter,.04);
    playSquareWave(E4,       measure * 7 + beat * 0 + time, quarter,.04);
    playSquareWave(G4,       measure * 7 + beat * 1 + time, quarter,.05);
    playSquareWave(Fsharp4,  measure * 7 + beat * 2 + time, quarter,.06);
    playSquareWave(E4,       measure * 7 + beat * 3 + time, quarter);
  }

  void part5_right_hand(float time){
    playSquareWave(D4,       measure * 0 + beat * 0 + time, quarter);
    playSquareWave(E4,       measure * 0 + beat * 1 + time, quarter);
    playSquareWave(Fsharp4,  measure * 0 + beat * 2 + time, quarter);
    playSquareWave(G4,       measure * 0 + beat * 3 + time, quarter);
    playSquareWave(A4,       measure * 1 + beat * 0 + time, whole,.03);
    playSquareWave(E4,       measure * 1 + beat * 1 + time, quarter);
    playSquareWave(A4,       measure * 1 + beat * 2 + time, quarter);
    playSquareWave(G4,       measure * 1 + beat * 3 + time, quarter);
    playSquareWave(Fsharp4,  measure * 2 + beat * 0 + time, eigth,0.05);
    playSquareWave(B4,       measure * 2 + beat * 1 + time, eigth,0.05);
    playSquareWave(A4,       measure * 2 + beat * 2 + time, eigth,0.05);
    playSquareWave(G4,       measure * 2 + beat * 3 + time, quarter,.09);
    playSquareWave(A4,       measure * 3 + beat * 0 + time, quarter,.07);
    playSquareWave(G4,       measure * 3 + beat * 1 + time, quarter,.05);
    playSquareWave(Fsharp4,  measure * 3 + beat * 2 + time, quarter,.03);
    playSquareWave(E4,       measure * 3 + beat * 3 + time, quarter,.03);

    playSquareWave(D4,       measure * 4 + beat * 0 + time, half,.05);
    playSquareWave(B4,       measure * 4 + beat * 2 + time, quarter,.06);
    playSquareWave(Csharp5,  measure * 4 + beat * 3 + time, quarter,.07);
    playSquareWave(D5,       measure * 5 + beat * 0 + time, quarter,.06);
    playSquareWave(D5,       measure * 5 + beat * 0 + time, whole,.02);
    playSquareWave(Csharp5,  measure * 5 + beat * 1 + time, quarter);
    playSquareWave(B4,       measure * 5 + beat * 2 + time, quarter,.06);
    playSquareWave(A4,       measure * 5 + beat * 3 + time, quarter,.05);
    playSquareWave(B4,       measure * 6 + beat * 0 + time, quarter,.06);
    playSquareWave(A4,       measure * 6 + beat * 1 + time, quarter,.06);
    playSquareWave(B4,       measure * 6 + beat * 2 + time, sixteenth,0.05);
    playSquareWave(D5,       measure * 6 + beat * 3 + time, sixteenth,0.05);
    playSquareWave(D5,       measure * 7 + beat * 0 + time, half,0.1);
    playSquareWave(Csharp5,  measure * 7 + beat * 2 + time, quarter,.05);
    playSquareWave(D5,       measure * 7 + beat * 3 + time, quarter);
  }

  void part6_right_hand(float time){
    playSquareWave(A5,       measure * 0 + beat * 0 + time, quarter,.09);
    playSquareWave(Fsharp5,  measure * 0 + beat * 1 + time, eigth,.05);
    playSquareWave(G5,       measure * 0 + beat * 1.5 + time, eigth,.06);
    playSquareWave(A5,       measure * 0 + beat * 2 + time, quarter,.09);
    playSquareWave(Fsharp5,  measure * 0 + beat * 3 + time, eigth);
    playSquareWave(G5,       measure * 0 + beat * 3.5 + time, eigth);

    playSquareWave(A5,       measure * 1 + beat * 0 + time, eigth,0.09);
    playSquareWave(A4,       measure * 1 + beat * 0.5 + time, eigth,.05);
    playSquareWave(B4,       measure * 1 + beat * 1 + time, eigth,.05);
    playSquareWave(Csharp5,  measure * 1 + beat * 1.5 + time, eigth,.05);
    playSquareWave(D5,       measure * 1 + beat * 2 + time, eigth,.05);
    playSquareWave(E5,       measure * 1 + beat * 2.5 + time, eigth,.05);
    playSquareWave(Fsharp5,  measure * 1 + beat * 3 + time, eigth,.05);
    playSquareWave(G5,       measure * 1 + beat * 3.5 + time, eigth,.05);

    playSquareWave(Fsharp5,  measure * 2 + beat * 0 + time, quarter,0.09);
    playSquareWave(D5,       measure * 2 + beat * 1 + time, eigth);
    playSquareWave(E5,       measure * 2 + beat * 1.5 + time, eigth, 0.06);
    playSquareWave(Fsharp5,  measure * 2 + beat * 2 + time, quarter,0.09);
    playSquareWave(Fsharp4,  measure * 2 + beat * 3 + time, eigth,.05);
    playSquareWave(G4,       measure * 2 + beat * 3.5 + time, eigth,.05);

    playSquareWave(A4,       measure * 3 + beat * 0 + time, eigth,.05);
    playSquareWave(B4,       measure * 3 + beat * 0.5 + time, eigth,.05);
    playSquareWave(A4,       measure * 3 + beat * 1 + time, eigth,.05);
    playSquareWave(G4,       measure * 3 + beat * 1.5 + time, eigth,.05);
    playSquareWave(A4,       measure * 3 + beat * 2 + time, eigth,.06);
    playSquareWave(Fsharp4,  measure * 3 + beat * 2.5 + time, eigth,.06);
    playSquareWave(G4,       measure * 3 + beat * 3 + time, eigth);
    playSquareWave(A4,       measure * 3 + beat * 3.5 + time, eigth);

    playSquareWave(G4,       measure * 4 + beat * 0 + time, quarter, .09);
    playSquareWave(B4,       measure * 4 + beat * 1 + time, eigth);
    playSquareWave(A4,       measure * 4 + beat * 1.5 + time, eigth ,.06);
    playSquareWave(G4,       measure * 4 + beat * 2 + time, quarter, .09);
    playSquareWave(Fsharp4,  measure * 4 + beat * 3 + time, eigth);
    playSquareWave(E4,       measure * 4 + beat * 3.5 + time, eigth, .06);

    playSquareWave(Fsharp4,  measure * 5 + beat * 0 + time, eigth,.05);
    playSquareWave(E4,       measure * 5 + beat * 0.5 + time, eigth,.05);
    playSquareWave(D4,       measure * 5 + beat * 1 + time, eigth,.04);
    playSquareWave(E4,       measure * 5 + beat * 1.5 + time, eigth,.04);
    playSquareWave(Fsharp4,  measure * 5 + beat * 2 + time, eigth,.04);
    playSquareWave(G4,       measure * 5 + beat * 2.5 + time, eigth,.04);
    playSquareWave(A4,       measure * 5 + beat * 3 + time, eigth,.06);
    playSquareWave(B4,       measure * 5 + beat * 3.5 + time, eigth,.07);

    playSquareWave(G4,       measure * 6 + beat * 0 + time, quarter);
    playSquareWave(B4,       measure * 6 + beat * 1 + time, eigth);
    playSquareWave(A4,       measure * 6 + beat * 1.5 + time, eigth);
    playSquareWave(B4,       measure * 6 + beat * 2 + time, quarter,.06);
    playSquareWave(Csharp5,  measure * 6 + beat * 3 + time, eigth,0.04);
    playSquareWave(D5,       measure * 6 + beat * 3.5 + time, eigth,0.04);

    playSquareWave(Csharp5,  measure * 7 + beat * 0 + time, eigth, 0.04);
    playSquareWave(A4,       measure * 7 + beat * 0.5 + time, eigth ,0.04);
    playSquareWave(B4,       measure * 7 + beat * 1 + time, eigth, 0.05);
    playSquareWave(Csharp5,  measure * 7 + beat * 1.5 + time, eigth, 0.05);
    playSquareWave(D5,       measure * 7 + beat * 2 + time, eigth, 0.06);
    playSquareWave(E5,       measure * 7 + beat * 2.5 + time, eigth, 0.07);
    playSquareWave(Fsharp5,  measure * 7 + beat * 3 + time, eigth,0.08);
    playSquareWave(G5,       measure * 7 + beat * 3.5 + time, eigth,.08);
  }

  void part6_right_hand_plunk(float time){
    playPluckString(A5,       measure * 0 + beat * 0 + time, quarter);
    playPluckString(Fsharp5,  measure * 0 + beat * 1 + time, eigth);
    playPluckString(G5,       measure * 0 + beat * 1.5 + time, eigth);
    playPluckString(A5,       measure * 0 + beat * 2 + time, quarter);
    playPluckString(Fsharp5,  measure * 0 + beat * 3 + time, eigth);
    playPluckString(G5,       measure * 0 + beat * 3.5 + time, eigth);

    playPluckString(A5,       measure * 1 + beat * 0 + time, eigth);
    playPluckString(A4,       measure * 1 + beat * 0.5 + time, eigth);
    playPluckString(B4,       measure * 1 + beat * 1 + time, eigth);
    playPluckString(Csharp5,  measure * 1 + beat * 1.5 + time, eigth);
    playPluckString(D5,       measure * 1 + beat * 2 + time, eigth);
    playPluckString(E5,       measure * 1 + beat * 2.5 + time, eigth);
    playPluckString(Fsharp5,  measure * 1 + beat * 3 + time, eigth);
    playPluckString(G5,       measure * 1 + beat * 3.5 + time, eigth);

    playPluckString(Fsharp5,  measure * 2 + beat * 0 + time, quarter);
    playPluckString(D5,       measure * 2 + beat * 1 + time, eigth);
    playPluckString(E5,       measure * 2 + beat * 1.5 + time, eigth);
    playPluckString(Fsharp5,  measure * 2 + beat * 2 + time, quarter);
    playPluckString(Fsharp4,  measure * 2 + beat * 3 + time, eigth);
    playPluckString(G4,       measure * 2 + beat * 3.5 + time, eigth);

    playPluckString(A4,       measure * 3 + beat * 0 + time, eigth);
    playPluckString(B4,       measure * 3 + beat * 0.5 + time, eigth);
    playPluckString(A4,       measure * 3 + beat * 1 + time, eigth);
    playPluckString(G4,       measure * 3 + beat * 1.5 + time, eigth);
    playPluckString(A4,       measure * 3 + beat * 2 + time, eigth);
    playPluckString(Fsharp4,  measure * 3 + beat * 2.5 + time, eigth);
    playPluckString(G4,       measure * 3 + beat * 3 + time, eigth);
    playPluckString(A4,       measure * 3 + beat * 3.5 + time, eigth);

    playPluckString(G4,       measure * 4 + beat * 0 + time, quarter);
    playPluckString(B4,       measure * 4 + beat * 1 + time, eigth);
    playPluckString(A4,       measure * 4 + beat * 1.5 + time, eigth);
    playPluckString(G4,       measure * 4 + beat * 2 + time, quarter);
    playPluckString(Fsharp4,  measure * 4 + beat * 3 + time, eigth);
    playPluckString(E4,       measure * 4 + beat * 3.5 + time, eigth);

    playPluckString(Fsharp4,  measure * 5 + beat * 0 + time, eigth);
    playPluckString(E4,       measure * 5 + beat * 0.5 + time, eigth);
    playPluckString(D4,       measure * 5 + beat * 1 + time, eigth);
    playPluckString(E4,       measure * 5 + beat * 1.5 + time, eigth);
    playPluckString(Fsharp4,  measure * 5 + beat * 2 + time, eigth);
    playPluckString(G4,       measure * 5 + beat * 2.5 + time, eigth);
    playPluckString(A4,       measure * 5 + beat * 3 + time, eigth);
    playPluckString(B4,       measure * 5 + beat * 3.5 + time, eigth);

    playPluckString(G4,       measure * 6 + beat * 0 + time, quarter);
    playPluckString(B4,       measure * 6 + beat * 1 + time, eigth);
    playPluckString(A4,       measure * 6 + beat * 1.5 + time, eigth);
    playPluckString(B4,       measure * 6 + beat * 2 + time, quarter);
    playPluckString(Csharp5,  measure * 6 + beat * 3 + time, eigth);
    playPluckString(D5,       measure * 6 + beat * 3.5 + time, eigth);

    playPluckString(Csharp5,  measure * 7 + beat * 0 + time, eigth);
    playPluckString(A4,       measure * 7 + beat * 0.5 + time, eigth);
    playPluckString(B4,       measure * 7 + beat * 1 + time, eigth);
    playPluckString(Csharp5,  measure * 7 + beat * 1.5 + time, eigth);
    playPluckString(D5,       measure * 7 + beat * 2 + time, eigth);
    playPluckString(E5,       measure * 7 + beat * 2.5 + time, eigth);
    playPluckString(Fsharp5,  measure * 7 + beat * 3 + time, eigth);
    playPluckString(G5,       measure * 7 + beat * 3.5 + time, eigth);
  }

  void part7_right_hand(float time){
    playSquareWave(A5,       measure * 0 + beat * 0 + time, quarter,.09);
    playSquareWave(Fsharp5,  measure * 0 + beat * 1 + time, eigth,.05);
    playSquareWave(G5,       measure * 0 + beat * 1.5 + time, eigth,.06);
    playSquareWave(A5,       measure * 0 + beat * 2 + time, quarter,.09);
    playSquareWave(Fsharp5,  measure * 0 + beat * 3 + time, eigth);
    playSquareWave(G5,       measure * 0 + beat * 3.5 + time, eigth);

    playSquareWave(A5,       measure * 1 + beat * 0 + time, eigth,0.09);
    playSquareWave(A4,       measure * 1 + beat * 0.5 + time, eigth,.05);
    playSquareWave(B4,       measure * 1 + beat * 1 + time, eigth,.05);
    playSquareWave(Csharp5,  measure * 1 + beat * 1.5 + time, eigth,.05);
    playSquareWave(D5,       measure * 1 + beat * 2 + time, eigth,.05);
    playSquareWave(E5,       measure * 1 + beat * 2.5 + time, eigth,.05);
    playSquareWave(Fsharp5,  measure * 1 + beat * 3 + time, eigth,.05);
    playSquareWave(G5,       measure * 1 + beat * 3.5 + time, eigth,.05);

    playSquareWave(Fsharp5,  measure * 2 + beat * 0 + time, quarter,0.09);
    playSquareWave(D5,       measure * 2 + beat * 1 + time, eigth);
    playSquareWave(E5,       measure * 2 + beat * 1.5 + time, eigth, 0.06);
    playSquareWave(Fsharp5,  measure * 2 + beat * 2 + time, quarter,0.09);
    playSquareWave(Fsharp4,  measure * 2 + beat * 3 + time, eigth,.05);
    playSquareWave(G4,       measure * 2 + beat * 3.5 + time, eigth,.05);

    playSquareWave(A4,       measure * 3 + beat * 0 + time, eigth,.05);
    playSquareWave(B4,       measure * 3 + beat * 0.5 + time, eigth,.05);
    playSquareWave(A4,       measure * 3 + beat * 1 + time, eigth,.05);
    playSquareWave(G4,       measure * 3 + beat * 1.5 + time, eigth,.05);
    playSquareWave(A4,       measure * 3 + beat * 2 + time, eigth,.06);
    playSquareWave(Fsharp4,  measure * 3 + beat * 2.5 + time, eigth,.06);
    playSquareWave(G4,       measure * 3 + beat * 3 + time, eigth);
    playSquareWave(A4,       measure * 3 + beat * 3.5 + time, eigth);

    playSquareWave(G4,       measure * 4 + beat * 0 + time, quarter, .09);
    playSquareWave(B4,       measure * 4 + beat * 1 + time, eigth);
    playSquareWave(A4,       measure * 4 + beat * 1.5 + time, eigth ,.06);
    playSquareWave(G4,       measure * 4 + beat * 2 + time, quarter, .09);
    playSquareWave(Fsharp4,  measure * 4 + beat * 3 + time, eigth);
    playSquareWave(E4,       measure * 4 + beat * 3.5 + time, eigth, .06);

    playSquareWave(Fsharp4,  measure * 5 + beat * 0 + time, eigth,.05);
    playSquareWave(E4,       measure * 5 + beat * 0.5 + time, eigth,.05);
    playSquareWave(D4,       measure * 5 + beat * 1 + time, eigth,.04);
    playSquareWave(E4,       measure * 5 + beat * 1.5 + time, eigth,.04);
    playSquareWave(Fsharp4,  measure * 5 + beat * 2 + time, eigth,.04);
    playSquareWave(G4,       measure * 5 + beat * 2.5 + time, eigth,.04);
    playSquareWave(A4,       measure * 5 + beat * 3 + time, eigth,.06);
    playSquareWave(B4,       measure * 5 + beat * 3.5 + time, eigth,.07);

    playSquareWave(G4,       measure * 6 + beat * 0 + time, quarter);
    playSquareWave(B4,       measure * 6 + beat * 1 + time, eigth);
    playSquareWave(A4,       measure * 6 + beat * 1.5 + time, eigth);
    playSquareWave(B4,       measure * 6 + beat * 2 + time, quarter,.06);
    playSquareWave(Csharp5,  measure * 6 + beat * 3 + time, eigth,0.04);
    playSquareWave(D5,       measure * 6 + beat * 3.5 + time, eigth,0.04);

    playSquareWave(Csharp5,  measure * 7 + beat * 0 + time, eigth, 0.04);
    playSquareWave(A4,       measure * 7 + beat * 0.5 + time, eigth ,0.04);
    playSquareWave(B4,       measure * 7 + beat * 1 + time, eigth, 0.05);
    playSquareWave(Csharp5,  measure * 7 + beat * 1.5 + time, eigth, 0.05);
    playSquareWave(D5,       measure * 7 + beat * 2 + time, eigth, 0.06);
    playSquareWave(E5,       measure * 7 + beat * 2.5 + time, eigth, 0.07);
    playSquareWave(Fsharp5,  measure * 7 + beat * 3 + time, eigth,0.08);
    playSquareWave(E5,       measure * 7 + beat * 3.5 + time, eigth,.08);

    playSquareWave(D5,       measure * 8 + beat * 0 + time, whole);
  }

  void part7_right_hand_plunk(float time){
    playPluckString(A5,       measure * 0 + beat * 0 + time, quarter);
    playPluckString(Fsharp5,  measure * 0 + beat * 1 + time, eigth);
    playPluckString(G5,       measure * 0 + beat * 1.5 + time, eigth);
    playPluckString(A5,       measure * 0 + beat * 2 + time, quarter);
    playPluckString(Fsharp5,  measure * 0 + beat * 3 + time, eigth);
    playPluckString(G5,       measure * 0 + beat * 3.5 + time, eigth);

    playPluckString(A5,       measure * 1 + beat * 0 + time, eigth);
    playPluckString(A4,       measure * 1 + beat * 0.5 + time, eigth);
    playPluckString(B4,       measure * 1 + beat * 1 + time, eigth);
    playPluckString(Csharp5,  measure * 1 + beat * 1.5 + time, eigth);
    playPluckString(D5,       measure * 1 + beat * 2 + time, eigth);
    playPluckString(E5,       measure * 1 + beat * 2.5 + time, eigth);
    playPluckString(Fsharp5,  measure * 1 + beat * 3 + time, eigth);
    playPluckString(G5,       measure * 1 + beat * 3.5 + time, eigth);

    playPluckString(Fsharp5,  measure * 2 + beat * 0 + time, quarter);
    playPluckString(D5,       measure * 2 + beat * 1 + time, eigth);
    playPluckString(E5,       measure * 2 + beat * 1.5 + time, eigth);
    playPluckString(Fsharp5,  measure * 2 + beat * 2 + time, quarter);
    playPluckString(Fsharp4,  measure * 2 + beat * 3 + time, eigth);
    playPluckString(G4,       measure * 2 + beat * 3.5 + time, eigth);

    playPluckString(A4,       measure * 3 + beat * 0 + time, eigth);
    playPluckString(B4,       measure * 3 + beat * 0.5 + time, eigth);
    playPluckString(A4,       measure * 3 + beat * 1 + time, eigth);
    playPluckString(G4,       measure * 3 + beat * 1.5 + time, eigth);
    playPluckString(A4,       measure * 3 + beat * 2 + time, eigth);
    playPluckString(Fsharp4,  measure * 3 + beat * 2.5 + time, eigth);
    playPluckString(G4,       measure * 3 + beat * 3 + time, eigth);
    playPluckString(A4,       measure * 3 + beat * 3.5 + time, eigth);

    playPluckString(G4,       measure * 4 + beat * 0 + time, quarter);
    playPluckString(B4,       measure * 4 + beat * 1 + time, eigth);
    playPluckString(A4,       measure * 4 + beat * 1.5 + time, eigth);
    playPluckString(G4,       measure * 4 + beat * 2 + time, quarter);
    playPluckString(Fsharp4,  measure * 4 + beat * 3 + time, eigth);
    playPluckString(E4,       measure * 4 + beat * 3.5 + time, eigth);

    playPluckString(Fsharp4,  measure * 5 + beat * 0 + time, eigth);
    playPluckString(E4,       measure * 5 + beat * 0.5 + time, eigth);
    playPluckString(D4,       measure * 5 + beat * 1 + time, eigth);
    playPluckString(E4,       measure * 5 + beat * 1.5 + time, eigth);
    playPluckString(Fsharp4,  measure * 5 + beat * 2 + time, eigth);
    playPluckString(G4,       measure * 5 + beat * 2.5 + time, eigth);
    playPluckString(A4,       measure * 5 + beat * 3 + time, eigth);
    playPluckString(B4,       measure * 5 + beat * 3.5 + time, eigth);

    playPluckString(G4,       measure * 6 + beat * 0 + time, quarter);
    playPluckString(B4,       measure * 6 + beat * 1 + time, eigth);
    playPluckString(A4,       measure * 6 + beat * 1.5 + time, eigth);
    playPluckString(B4,       measure * 6 + beat * 2 + time, quarter);
    playPluckString(Csharp5,  measure * 6 + beat * 3 + time, eigth);
    playPluckString(D5,       measure * 6 + beat * 3.5 + time, eigth);

    playPluckString(Csharp5,  measure * 7 + beat * 0 + time, eigth);
    playPluckString(A4,       measure * 7 + beat * 0.5 + time, eigth);
    playPluckString(B4,       measure * 7 + beat * 1 + time, eigth);
    playPluckString(Csharp5,  measure * 7 + beat * 1.5 + time, eigth);
    playPluckString(D5,       measure * 7 + beat * 2 + time, eigth);
    playPluckString(E5,       measure * 7 + beat * 2.5 + time, eigth);
    playPluckString(Fsharp5,  measure * 7 + beat * 3 + time, eigth);
    playPluckString(E5,       measure * 7 + beat * 3.5 + time, eigth);

    playPluckString(D5,       measure * 8 + beat * 0 + time, whole);
  }

  void part8_left_hand(float time){
    playSineEnv(D3,       measure * 0 + beat * 0 + time, quarter,.06);
    playSineEnv(Fsharp3,  measure * 0 + beat * 0 + time, whole,.06);
    playSineEnv(A3,       measure * 0 + beat * 2 + time, quarter,.075);
    playSineEnv(D4,       measure * 0 + beat * 3 + time, quarter,.09);
  }

  void hihat(float time){
    playHihat(measure * 0 + beat * 0 + time);
    playHihat(measure * 1 + beat * 0 + time);
    playHihat(measure * 2 + beat * 0 + time);
    playHihat(measure * 3 + beat * 0 + time);
    playHihat(measure * 4 + beat * 0 + time);
    playHihat(measure * 5 + beat * 0 + time);
    playHihat(measure * 6 + beat * 0 + time);
    playHihat(measure * 7 + beat * 0 + time);
  }
  

  void playSong(){
    part1(0);
    part2(0 + 8*measure);
    part3_left(0 + 16*measure);
    part3_right(0 + 16*measure);
    part4_left_hand(0 + 24*measure);
    part4_right_hand(0 + 24*measure);
    part4_left_hand(0 + 32*measure);
    part5_right_hand(0 + 32*measure);
    part4_left_hand(0 + 40*measure);
    part6_right_hand(0 + 40*measure);
    part6_right_hand_plunk(0 + 40*measure);
    part4_left_hand(0 + 48*measure);
    part7_right_hand(0 + 48*measure);
    part7_right_hand_plunk(0 + 48*measure);
    part8_left_hand(0 + 56*measure);

    hihat(0 + 32*measure);
    hihat(0 + 40*measure);
    hihat(0 + 40*measure + 2*beat);
    hihat(0 + 48*measure);
    hihat(0 + 48*measure + 1*beat);
    hihat(0 + 48*measure + 2*beat);
    hihat(0 + 48*measure + 3*beat);
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