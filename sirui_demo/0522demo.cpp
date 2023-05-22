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
#include "al_ext/assets3d/al_Asset.hpp"

#include <cmath>
#include <algorithm> 
#include <cstdint>   
#include <vector>

using namespace gam;
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
  gam::Sine<> mOsc5;
  gam::Sine<> mOsc7;
  gam::Env<3> mAmpEnv;
  gam::Saw<> mSaw1;

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
    mAmpEnv.levels(0, 1, 1, 0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    // This is a quick way to create parameters for the voice. Trigger
    // parameters are meant to be set only when the voice starts, i.e. they
    // are expected to be constant within a voice instance. (You can actually
    // change them while you are prototyping, but their changes will only be
    // stored and aplied when a note is triggered.)

    //addCone(mMesh);
    addDodecahedron(mMesh,0.4);
    addCircle(mMesh, 0.77);

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
    mSaw1.freq(getInternalParameterValue("frequency"));
  

    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io())
    {
      
      float s1 =  // mSaw1() * (1.0) * mAmpEnv() * getInternalParameterValue("amplitude")
              // + mSaw3() * (1.0/6.0) * mAmpEnv() * getInternalParameterValue("amplitude")
              //mSaw2() * (1.0/2.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               mOsc1() * (1.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc3() * (1.0/3.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc5() * (1.0/5.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc7() * (1.0/7.0) * mAmpEnv() * getInternalParameterValue("amplitude");
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
    float release = getInternalParameterValue("releaseTime");
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
  void onTriggerOn() override { mAmpEnv.reset();}

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


  void playSineEnv(float freq, float time, float duration, float amp, float attack = 0.2, float decay = 0.2)
  {
    auto *voice = synthManager.synth().getVoice<SineEnv>();
    // amp, freq, attack, release, pan
    vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
    voice->setTriggerParams(params);
    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  void playPluckString(float freq, float time, float duration, float amp = 0.07)
  {
    
    auto *voice = synthManager.synth().getVoice<PluckedString>();
    //vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
    //voice->setTriggerParams(params);

    voice->setInternalParameterValue("frequency", freq);
    voice->setInternalParameterValue("amplitude", amp);

    synthManager.synthSequencer().addVoiceFromNow(voice, time, duration);
  }

  // NOTES AND CORRESPONDING FREQUENCIES
  const float C6 = 1046.50;
  const float D6 = 1174.66;
  const float E6 = 1318.51;
  const float Fsharp6 = 1479.98;
  const float G6 = 1567.98;
  const float A6 = 1760.00;
  const float B6 = 1975.53;

  const float C5 = 523.25;
  const float D5 = 587.33;
  const float E5 = 659.25;
  const float Fsharp5 = 739.99;
  const float G5 = 783.99;
  const float A5 = 880.0;
  const float B5 = 987.77;

  const float C4 = 261.63;
  const float D4 = 293.66;
  const float E4 = 329.63;
  const float Fsharp4 = 369.99;
  const float G4 = 392.0;
  const float A4 = 440.0;
  const float B4 = 493.88;

  const float C3 = C4 / 2;
  const float D3 = D4 / 2;
  const float E3 = E4 / 2;
  const float Fsharp3 = Fsharp4 / 2;
  const float G3 = G4 / 2;
  const float A3 = A4 / 2;
  const float B3 = B4 / 2;

  const float C2 = C3 / 2;
  const float D2 = D3 / 2;
  const float E2 = E3 / 2;
  const float Fsharp2 = Fsharp3 / 2;
  const float G2 = G3 / 2;
  const float A2 = A3 / 2;
  const float B2 = B3 / 2;

  const float G1 = G2 / 2;
  const float A1 = A2 / 2;
  const float B1 = B2 / 2;

  // from https://github.com/allolib-s23/demo1-christinetu15/blob/main/tutorials/synthesis/demo-christine.cpp
  const float bpm = 105;
  const float beat = 60 / bpm;
  const float measure = beat * 4;

  const float whole = measure;
  const float half = beat * 2;
  const float quarter = beat;
  const float eigth = quarter / 2;
  const float sixteenth = eigth / 2;

  void chord(float time){
    playSineEnv(D4, measure * 0 + beat * 0 + time, whole, 0.05);
    playSineEnv(B3, measure * 0 + beat * 0 + time, whole, 0.05);
    playSineEnv(G3, measure * 0 + beat * 0 + time, whole, 0.07);

    playSineEnv(E4, measure * 1 + beat * 0 + time, whole, 0.05);
    playSineEnv(C4, measure * 1 + beat * 0 + time, whole, 0.05);
    playSineEnv(G3, measure * 1 + beat * 0 + time, whole, 0.07);

    playSineEnv(D4, measure * 2 + beat * 0 + time, whole, 0.05);
    playSineEnv(G4, measure * 2 + beat * 0 + time, whole, 0.05);
    playSineEnv(B3, measure * 2 + beat * 0 + time, whole, 0.07);

    playSineEnv(A4, measure * 3 + beat * 0 + time, whole, 0.05);
    playSineEnv(E4, measure * 3 + beat * 0 + time, whole, 0.05);
    playSineEnv(C4, measure * 3 + beat * 0 + time, whole, 0.07);

  }

  void melody1(float time){
    playPluckString(B4, measure * 0 + beat * 0 + time, sixteenth);  //
    playPluckString(D5, measure * 0 + beat * 0.25 + time, sixteenth);
    playPluckString(G5, measure * 0 + beat * 0.5 + time, sixteenth);
    playPluckString(D5, measure * 0 + beat * 0.75 + time, sixteenth);

    playPluckString(B4, measure * 0 + beat * 1 + time, sixteenth);
    playPluckString(D5, measure * 0 + beat * 1.25 + time, sixteenth);
    playPluckString(G5, measure * 0 + beat * 1.5 + time, sixteenth);
    playPluckString(D5, measure * 0 + beat * 1.75 + time, sixteenth);

    playPluckString(B4, measure * 0 + beat * 2 + time, sixteenth);
    playPluckString(D5, measure * 0 + beat * 2.25 + time, sixteenth);
    playPluckString(G5, measure * 0 + beat * 2.5 + time, sixteenth);
    playPluckString(D5, measure * 0 + beat * 2.75 + time, sixteenth);

    playPluckString(B4, measure * 0 + beat * 3 + time, sixteenth);
    playPluckString(D5, measure * 0 + beat * 3.25 + time, sixteenth);
    playPluckString(G5, measure * 0 + beat * 3.5 + time, sixteenth);
    playPluckString(D5, measure * 0 + beat * 3.75 + time, sixteenth);

    playPluckString(C5, measure * 1 + beat * 0 + time, sixteenth);  //
    playPluckString(E5, measure * 1 + beat * 0.25 + time, sixteenth);
    playPluckString(A5, measure * 1 + beat * 0.5 + time, sixteenth);
    playPluckString(E5, measure * 1 + beat * 0.75 + time, sixteenth);

    playPluckString(C5, measure * 1 + beat * 1 + time, sixteenth);
    playPluckString(E5, measure * 1 + beat * 1.25 + time, sixteenth);
    playPluckString(A5, measure * 1 + beat * 1.5 + time, sixteenth);
    playPluckString(E5, measure * 1 + beat * 1.75 + time, sixteenth);

    playPluckString(C5, measure * 1 + beat * 2 + time, sixteenth);
    playPluckString(E5, measure * 1 + beat * 2.25 + time, sixteenth);
    playPluckString(A5, measure * 1 + beat * 2.5 + time, sixteenth);
    playPluckString(E5, measure * 1 + beat * 2.75 + time, sixteenth);

    playPluckString(C5, measure * 1 + beat * 3 + time, sixteenth);
    playPluckString(E5, measure * 1 + beat * 3.25 + time, sixteenth);
    playPluckString(A5, measure * 1 + beat * 3.5 + time, sixteenth);
    playPluckString(E5, measure * 1 + beat * 3.75 + time, sixteenth);

    playPluckString(D5, measure * 2 + beat * 0 + time, sixteenth);  //
    playPluckString(G5, measure * 2 + beat * 0.25 + time, sixteenth);
    playPluckString(B5, measure * 2 + beat * 0.5 + time, sixteenth);
    playPluckString(G5, measure * 2 + beat * 0.75 + time, sixteenth);

    playPluckString(D5, measure * 2 + beat * 1 + time, sixteenth);
    playPluckString(G5, measure * 2 + beat * 1.25 + time, sixteenth);
    playPluckString(B5, measure * 2 + beat * 1.5 + time, sixteenth);
    playPluckString(G5, measure * 2 + beat * 1.75 + time, sixteenth);

    playPluckString(D5, measure * 2 + beat * 2 + time, sixteenth);
    playPluckString(G5, measure * 2 + beat * 2.25 + time, sixteenth);
    playPluckString(B5, measure * 2 + beat * 2.5 + time, sixteenth);
    playPluckString(G5, measure * 2 + beat * 2.75 + time, sixteenth);

    playPluckString(D5, measure * 2 + beat * 3 + time, sixteenth);
    playPluckString(G5, measure * 2 + beat * 3.25 + time, sixteenth);
    playPluckString(B5, measure * 2 + beat * 3.5 + time, sixteenth);
    playPluckString(G5, measure * 2 + beat * 3.75 + time, sixteenth);

    playPluckString(E5, measure * 3 + beat * 0 + time, sixteenth);  //
    playPluckString(A5, measure * 3 + beat * 0.25 + time, sixteenth);
    playPluckString(C6, measure * 3 + beat * 0.5 + time, sixteenth);
    playPluckString(A5, measure * 3 + beat * 0.75 + time, sixteenth);

    playPluckString(E5, measure * 3 + beat * 1 + time, sixteenth);
    playPluckString(A5, measure * 3 + beat * 1.25 + time, sixteenth);
    playPluckString(C6, measure * 3 + beat * 1.5 + time, sixteenth);
    playPluckString(A5, measure * 3 + beat * 1.75 + time, sixteenth);

    playPluckString(E5, measure * 3 + beat * 2 + time, sixteenth);
    playPluckString(A5, measure * 3 + beat * 2.25 + time, sixteenth);
    playPluckString(C6, measure * 3 + beat * 2.5 + time, sixteenth);
    playPluckString(A5, measure * 3 + beat * 2.75 + time, sixteenth);

    playPluckString(E5, measure * 3 + beat * 3 + time, sixteenth);
    playPluckString(A5, measure * 3 + beat * 3.25 + time, sixteenth);
    playPluckString(C6, measure * 3 + beat * 3.5 + time, sixteenth);
    playPluckString(A5, measure * 3 + beat * 3.75 + time, sixteenth);
  }

  void melody1_lower(float time){
    playPluckString(B3, measure * 0 + beat * 0 + time, sixteenth);  //
    playPluckString(D4, measure * 0 + beat * 0.25 + time, sixteenth);
    playPluckString(G4, measure * 0 + beat * 0.5 + time, sixteenth);
    playPluckString(D4, measure * 0 + beat * 0.75 + time, sixteenth);

    playPluckString(B3, measure * 0 + beat * 1 + time, sixteenth);
    playPluckString(D4, measure * 0 + beat * 1.25 + time, sixteenth);
    playPluckString(G4, measure * 0 + beat * 1.5 + time, sixteenth);
    playPluckString(D4, measure * 0 + beat * 1.75 + time, sixteenth);

    playPluckString(B3, measure * 0 + beat * 2 + time, sixteenth);
    playPluckString(D4, measure * 0 + beat * 2.25 + time, sixteenth);
    playPluckString(G4, measure * 0 + beat * 2.5 + time, sixteenth);
    playPluckString(D4, measure * 0 + beat * 2.75 + time, sixteenth);

    playPluckString(B3, measure * 0 + beat * 3 + time, sixteenth);
    playPluckString(D4, measure * 0 + beat * 3.25 + time, sixteenth);
    playPluckString(G4, measure * 0 + beat * 3.5 + time, sixteenth);
    playPluckString(D4, measure * 0 + beat * 3.75 + time, sixteenth);

    playPluckString(C4, measure * 1 + beat * 0 + time, sixteenth);  //
    playPluckString(E4, measure * 1 + beat * 0.25 + time, sixteenth);
    playPluckString(A4, measure * 1 + beat * 0.5 + time, sixteenth);
    playPluckString(E4, measure * 1 + beat * 0.75 + time, sixteenth);

    playPluckString(C4, measure * 1 + beat * 1 + time, sixteenth);
    playPluckString(E4, measure * 1 + beat * 1.25 + time, sixteenth);
    playPluckString(A4, measure * 1 + beat * 1.5 + time, sixteenth);
    playPluckString(E4, measure * 1 + beat * 1.75 + time, sixteenth);

    playPluckString(C4, measure * 1 + beat * 2 + time, sixteenth);
    playPluckString(E4, measure * 1 + beat * 2.25 + time, sixteenth);
    playPluckString(A4, measure * 1 + beat * 2.5 + time, sixteenth);
    playPluckString(E4, measure * 1 + beat * 2.75 + time, sixteenth);

    playPluckString(C4, measure * 1 + beat * 3 + time, sixteenth);
    playPluckString(E4, measure * 1 + beat * 3.25 + time, sixteenth);
    playPluckString(A4, measure * 1 + beat * 3.5 + time, sixteenth);
    playPluckString(E4, measure * 1 + beat * 3.75 + time, sixteenth);

    playPluckString(D4, measure * 2 + beat * 0 + time, sixteenth);  //
    playPluckString(G4, measure * 2 + beat * 0.25 + time, sixteenth);
    playPluckString(B4, measure * 2 + beat * 0.5 + time, sixteenth);
    playPluckString(G4, measure * 2 + beat * 0.75 + time, sixteenth);

    playPluckString(D4, measure * 2 + beat * 1 + time, sixteenth);
    playPluckString(G4, measure * 2 + beat * 1.25 + time, sixteenth);
    playPluckString(B4, measure * 2 + beat * 1.5 + time, sixteenth);
    playPluckString(G4, measure * 2 + beat * 1.75 + time, sixteenth);

    playPluckString(D4, measure * 2 + beat * 2 + time, sixteenth);
    playPluckString(G4, measure * 2 + beat * 2.25 + time, sixteenth);
    playPluckString(B4, measure * 2 + beat * 2.5 + time, sixteenth);
    playPluckString(G4, measure * 2 + beat * 2.75 + time, sixteenth);

    playPluckString(D4, measure * 2 + beat * 3 + time, sixteenth);
    playPluckString(G4, measure * 2 + beat * 3.25 + time, sixteenth);
    playPluckString(B4, measure * 2 + beat * 3.5 + time, sixteenth);
    playPluckString(G4, measure * 2 + beat * 3.75 + time, sixteenth);

    playPluckString(E4, measure * 3 + beat * 0 + time, sixteenth);  //
    playPluckString(A4, measure * 3 + beat * 0.25 + time, sixteenth);
    playPluckString(C5, measure * 3 + beat * 0.5 + time, sixteenth);
    playPluckString(A4, measure * 3 + beat * 0.75 + time, sixteenth);

    playPluckString(E4, measure * 3 + beat * 1 + time, sixteenth);
    playPluckString(A4, measure * 3 + beat * 1.25 + time, sixteenth);
    playPluckString(C5, measure * 3 + beat * 1.5 + time, sixteenth);
    playPluckString(A4, measure * 3 + beat * 1.75 + time, sixteenth);

    playPluckString(E4, measure * 3 + beat * 2 + time, sixteenth);
    playPluckString(A4, measure * 3 + beat * 2.25 + time, sixteenth);
    playPluckString(C5, measure * 3 + beat * 2.5 + time, sixteenth);
    playPluckString(A4, measure * 3 + beat * 2.75 + time, sixteenth);

    playPluckString(E4, measure * 3 + beat * 3 + time, sixteenth);
    playPluckString(A4, measure * 3 + beat * 3.25 + time, sixteenth);
    playPluckString(C5, measure * 3 + beat * 3.5 + time, sixteenth);
    playPluckString(A4, measure * 3 + beat * 3.75 + time, sixteenth);
  }

  void playSong(){
    chord(1);
    melody1(1);
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