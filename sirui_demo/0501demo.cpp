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

class FM : public SynthVoice {
 public:
  // Unit generators
  gam::Pan<> mPan;
  gam::ADSR<> mAmpEnv;
  gam::ADSR<> mModEnv;
  gam::EnvFollow<> mEnvFollow;

  gam::Sine<> car, mod;  // carrier, modulator sine oscillators

  // Additional members
  Mesh mMesh;

  void init() override {
    //      mAmpEnv.curve(0); // linear segments
    mAmpEnv.levels(0, 1, 1, 0);

    // We have the mesh be a sphere
    addDisc(mMesh, 1.0, 30);

    createInternalTriggerParameter("freq", 440, 10, 4000.0);
    createInternalTriggerParameter("amplitude", 0.5, 0.0, 1.0);
    createInternalTriggerParameter("attackTime", 0.1, 0.01, 3.0);
    createInternalTriggerParameter("releaseTime", 0.1, 0.1, 10.0);
    createInternalTriggerParameter("sustain", 0.75, 0.1, 1.0);  // Unused

    // FM index
    createInternalTriggerParameter("idx1", 0.01, 0.0, 10.0);
    createInternalTriggerParameter("idx2", 7, 0.0, 10.0);
    createInternalTriggerParameter("idx3", 5, 0.0, 10.0);

    createInternalTriggerParameter("carMul", 1, 0.0, 20.0);
    createInternalTriggerParameter("modMul", 1.0007, 0.0, 20.0);

    createInternalTriggerParameter("pan", 0.0, -1.0, 1.0);
  }

  //
  void onProcess(AudioIOData& io) override {
    float modFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    mod.freq(modFreq);
    float carBaseFreq =
        getInternalParameterValue("freq") * getInternalParameterValue("carMul");
    float modScale =
        getInternalParameterValue("freq") * getInternalParameterValue("modMul");
    float amp = getInternalParameterValue("amplitude");
    while (io()) {
      car.freq(carBaseFreq + mod() * mModEnv() * modScale);
      float s1 = car() * mAmpEnv() * amp;
      float s2;
      mEnvFollow(s1);
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    if (mAmpEnv.done() && (mEnvFollow.value() < 0.001)) free();
  }

  void onProcess(Graphics& g) override {
    g.pushMatrix();
    g.translate(getInternalParameterValue("freq") / 300 - 2,
                getInternalParameterValue("modAmt") / 25 - 1, -4);
    float scaling = getInternalParameterValue("amplitude") * 1;
    g.scale(scaling, scaling, scaling * 1);
    g.color(HSV(getInternalParameterValue("modMul") / 20, 1,
                mEnvFollow.value() * 10));
    g.draw(mMesh);
    g.popMatrix();
  }

  void onTriggerOn() override {
    mModEnv.levels()[0] = getInternalParameterValue("idx1");
    mModEnv.levels()[1] = getInternalParameterValue("idx2");
    mModEnv.levels()[2] = getInternalParameterValue("idx2");
    mModEnv.levels()[3] = getInternalParameterValue("idx3");

    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mModEnv.lengths()[0] = getInternalParameterValue("attackTime");

    mAmpEnv.lengths()[1] = 0.001;
    mModEnv.lengths()[1] = 0.001;

    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mModEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));

    //        mModEnv.lengths()[1] = mAmpEnv.lengths()[1];

    mAmpEnv.reset();
    mModEnv.reset();
  }
  void onTriggerOff() override {
    mAmpEnv.triggerRelease();
    mModEnv.triggerRelease();
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


// We make an app.
class MyApp : public App {
public:
  // GUI manager for SineEnv voices
  // The name provided determines the name of the directory
  // where the presets and sequences are stored
  SynthGUIManager<FM> synthManager{"synth4"};
  // SynthGUIManager<SquareWave> synthManager{"SquareWave"};
  ParameterMIDI parameterMIDI;
  int midiNote;

  void onInit() override {
    // Set sampling rate for Gamma objects from app's audio
    gam::sampleRate(audioIO().framesPerSecond());
  }

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
      synthManager.triggerOff(midiNote - 24);
    }
    return true;
  }

  void onExit() override { imguiShutdown(); }

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



  void playSong(){
    
    for(int i=16; i<24; i++) {
      if(i % 8 == 0) {
        riserSnare(i);
      }
    }
    for(int i=24; i<40; i++) {
      rhythmSnare(i);
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