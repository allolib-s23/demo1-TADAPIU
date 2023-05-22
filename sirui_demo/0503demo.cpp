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
    addDodecahedron(mMesh);

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
    // Now draw
    g.pushMatrix();
    // Move x according to frequency, y according to amplitude
    g.translate(frequency / 300 - 2, 1, -8);
    // Scale in the x and y directions according to amplitude
    g.scale(0.5 * amplitude, 5 * amplitude, 1);
    // Set the color. Red and Blue according to sound amplitude and Green
    // according to frequency. Alpha fixed to 0.4
    g.color(frequency / 1000, amplitude / 1000,  mEnvFollow.value(), 0.4);
    g.draw(mMesh);
    //g.popMatrix();
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


  void playSineEnv(float freq, float time, float duration, float amp = .1, float attack = 0.2, float decay = 0.2)
  {
    auto *voice = synthManager.synth().getVoice<SineEnv>();
    // amp, freq, attack, release, pan
    vector<VariantValue> params = vector<VariantValue>({amp, freq, attack, decay, 0.0});
    voice->setTriggerParams(params);
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

  // from https://github.com/allolib-s23/demo1-christinetu15/blob/main/tutorials/synthesis/demo-christine.cpp
  const float bpm = 140;
  const float beat = 60 / bpm;
  const float measure = beat * 4;

  const float whole = measure;
  const float half = beat * 2;
  const float quarter = beat;
  const float eigth = quarter / 2;
  const float sixteenth = eigth / 2;

  void part1(float time){
    playSineEnv(Fsharp4, measure * 0 + beat * 0 + time, whole);
    playSineEnv(D3, measure * 0 + beat * 0 + time, whole);
    playSineEnv(E4, measure * 1 + beat * 0 + time, whole);
    playSineEnv(A2, measure * 1 + beat * 0 + time, whole);
    playSineEnv(D4, measure * 2 + beat * 0 + time, whole);
    playSineEnv(B2, measure * 2 + beat * 0 + time, whole);
    playSineEnv(Csharp4, measure * 3 + beat * 0 + time, whole);
    playSineEnv(Fsharp2, measure * 3 + beat * 0 + time, whole);
    playSineEnv(B3, measure * 4 + beat * 0 + time, whole);
    playSineEnv(G2, measure * 4 + beat * 0 + time, whole);
    playSineEnv(A3, measure * 5 + beat * 0 + time, whole);
    playSineEnv(D2, measure * 5 + beat * 0 + time, whole);
    playSineEnv(B3, measure * 6 + beat * 0 + time, whole);
    playSineEnv(G2, measure * 6 + beat * 0 + time, whole);
    playSineEnv(Csharp4, measure * 7 + beat * 0 + time, whole);
    playSineEnv(A2, measure * 7 + beat * 0 + time, whole);
  }

  void part2(float time){
    playSineEnv(Fsharp5, measure * 0 + beat * 0 + time, whole);
    playSineEnv(D5, measure * 0 + beat * 0 + time, whole);
    playSineEnv(D4, measure * 0 + beat * 0 + time, whole);
    playSineEnv(D3, measure * 0 + beat * 0 + time, whole);
    playSineEnv(Csharp5, measure * 1 + beat * 0 + time, whole);
    playSineEnv(E5, measure * 1 + beat * 0 + time, whole);
    playSineEnv(A3, measure * 1 + beat * 0 + time, whole);
    playSineEnv(A2, measure * 1 + beat * 0 + time, whole);
    playSineEnv(G4, measure * 1 + beat * 2 + time, half);
    playSineEnv(D5, measure * 2 + beat * 0 + time, whole);
    playSineEnv(B4, measure * 2 + beat * 0 + time, whole);
    playSineEnv(B3, measure * 2 + beat * 0 + time, whole);
    playSineEnv(B2, measure * 2 + beat * 0 + time, whole);
    playSineEnv(Csharp5, measure * 3 + beat * 0 + time, whole);
    playSineEnv(A4, measure * 3 + beat * 0 + time, whole);
    playSineEnv(Fsharp3, measure * 3 + beat * 0 + time, whole);
    playSineEnv(Fsharp2, measure * 3 + beat * 0 + time, whole);
    playSineEnv(E4, measure * 3 + beat * 2 + time, half);
    playSineEnv(B4, measure * 4 + beat * 0 + time, whole);
    playSineEnv(D4, measure * 4 + beat * 0 + time, whole);
    playSineEnv(G3, measure * 4 + beat * 0 + time, whole);
    playSineEnv(G2, measure * 4 + beat * 0 + time, whole);
    playSineEnv(A4, measure * 5 + beat * 0 + time, half);
    playSineEnv(Fsharp4, measure * 5 + beat * 0 + time, whole);
    playSineEnv(D3, measure * 5 + beat * 0 + time, whole);
    playSineEnv(D2, measure * 5 + beat * 0 + time, whole);
    playSineEnv(A4, measure * 5 + beat * 2 + time, half);
    playSineEnv(B4, measure * 6 + beat * 0 + time, whole);
    playSineEnv(G4, measure * 6 + beat * 0 + time, whole);
    playSineEnv(G3, measure * 6 + beat * 0 + time, whole);
    playSineEnv(G2, measure * 6 + beat * 0 + time, whole);
    playSineEnv(Csharp5, measure * 7 + beat * 0 + time, whole);
    playSineEnv(A4, measure * 7 + beat * 0 + time, whole);
    playSineEnv(A3, measure * 7 + beat * 0 + time, whole);
    playSineEnv(A2, measure * 7 + beat * 0 + time, whole);
    playSineEnv(G4, measure * 7 + beat * 2 + time, half);

  }

  // void part3(float time){
  //   playSineEnv(Fsharp5,  measure * 0 + beat * 0 + time, whole);
  //   playSineEnv(D3,       measure * 0 + beat * 0 + time, whole);
  //   playSineEnv(A3,       measure * 0 + beat * 1 + time, quarter);
  //   playSineEnv(D4,       measure * 0 + beat * 2 + time, half);
  //   playSineEnv(E5,       measure * 1 + beat * 0 + time, whole);
  //   playSineEnv(A2,       measure * 1 + beat * 0 + time, whole);
  //   playSineEnv(E3,       measure * 1 + beat * 1 + time, quarter);
  //   playSineEnv(G3,       measure * 1 + beat * 2 + time, half);
  //   playSineEnv(D5,       measure * 2 + beat * 0 + time, whole);
  //   playSineEnv(B2,       measure * 2 + beat * 0 + time, whole);
  //   playSineEnv(Fsharp3,  measure * 2 + beat * 1 + time, quarter);
  //   playSineEnv(B3,       measure * 2 + beat * 2 + time, half);
  //   playSineEnv(Csharp5,  measure * 3 + beat * 0 + time, whole);
  //   playSineEnv(Fsharp2,  measure * 3 + beat * 0 + time, whole);
  //   playSineEnv(Csharp3,  measure * 3 + beat * 1 + time, quarter);
  //   playSineEnv(Fsharp3,  measure * 3 + beat * 2 + time, half);
  //   playSineEnv(B4,       measure * 4 + beat * 0 + time, whole);
  //   playSineEnv(G2,       measure * 4 + beat * 0 + time, whole);
  //   playSineEnv(D3,       measure * 4 + beat * 1 + time, quarter);
  //   playSineEnv(G3,       measure * 4 + beat * 2 + time, half);
  //   playSineEnv(A4,       measure * 5 + beat * 0 + time, whole);
  //   playSineEnv(D2,       measure * 5 + beat * 0 + time, whole);
  //   playSineEnv(A2,       measure * 5 + beat * 1 + time, quarter);
  //   playSineEnv(D3,       measure * 5 + beat * 2 + time, half);
  //   playSineEnv(B4,       measure * 6 + beat * 0 + time, whole);
  //   playSineEnv(G2,       measure * 6 + beat * 0 + time, whole);
  //   playSineEnv(D3,       measure * 6 + beat * 1 + time, quarter);
  //   playSineEnv(G3,       measure * 6 + beat * 2 + time, half);
  //   playSineEnv(Csharp5,  measure * 7 + beat * 0 + time, whole);
  //   playSineEnv(A2,       measure * 7 + beat * 0 + time, whole);
  //   playSineEnv(E3,       measure * 7 + beat * 1 + time, quarter);
  //   playSineEnv(A3,       measure * 7 + beat * 2 + time, half);
  // }

  void part3(float time){
    playSineEnv(Fsharp5,  measure * 0 + beat * 0 + time, whole);
    playSineEnv(D3,       measure * 0 + beat * 0 + time, whole);
    playSineEnv(A3,       measure * 0 + beat * 1 + time, quarter);
    playSineEnv(D4,       measure * 0 + beat * 2 + time, half);
    playSineEnv(E5,       measure * 1 + beat * 0 + time, whole);
    playSineEnv(A2,       measure * 1 + beat * 0 + time, whole);
    playSineEnv(E3,       measure * 1 + beat * 1 + time, quarter);
    playSineEnv(G3,       measure * 1 + beat * 2 + time, half);
    playSineEnv(D5,       measure * 2 + beat * 0 + time, whole);
    playSineEnv(B2,       measure * 2 + beat * 0 + time, whole);
    playSineEnv(Fsharp3,  measure * 2 + beat * 1 + time, quarter);
    playSineEnv(B3,       measure * 2 + beat * 2 + time, half);
    playSineEnv(Csharp5,  measure * 3 + beat * 0 + time, whole);
    playSineEnv(Fsharp2,  measure * 3 + beat * 0 + time, whole);
    playSineEnv(Csharp3,  measure * 3 + beat * 1 + time, quarter);
    playSineEnv(Fsharp3,  measure * 3 + beat * 2 + time, half);

    playSineEnv(B4,       measure * 4 + beat * 0 + time, whole);
    playSineEnv(G4,       measure * 4 + beat * 0 + time, whole);
    playSineEnv(G2,       measure * 4 + beat * 0 + time, whole);
    playSineEnv(D3,       measure * 4 + beat * 1 + time, quarter);
    playSineEnv(G3,       measure * 4 + beat * 2 + time, quarter);
    playSineEnv(D3,       measure * 4 + beat * 3 + time, quarter);

    playSineEnv(A4,       measure * 5 + beat * 0 + time, whole);
    playSineEnv(Fsharp4,  measure * 5 + beat * 0 + time, whole);
    playSineEnv(D2,       measure * 5 + beat * 0 + time, whole);
    playSineEnv(A2,       measure * 5 + beat * 1 + time, quarter);
    playSineEnv(D3,       measure * 5 + beat * 2 + time, quarter);
    playSineEnv(A2,       measure * 5 + beat * 3 + time, quarter);

    playSineEnv(B4,       measure * 6 + beat * 0 + time, whole);
    playSineEnv(G4,       measure * 6 + beat * 0 + time, whole);
    playSineEnv(G2,       measure * 6 + beat * 0 + time, whole);
    playSineEnv(D3,       measure * 6 + beat * 1 + time, quarter);
    playSineEnv(G3,       measure * 6 + beat * 2 + time, quarter);
    playSineEnv(D3,       measure * 6 + beat * 3 + time, quarter);

    playSineEnv(Csharp5,  measure * 7 + beat * 0 + time, whole);
    playSineEnv(A4,       measure * 7 + beat * 0 + time, whole);
    playSineEnv(A2,       measure * 7 + beat * 0 + time, whole);
    playSineEnv(E3,       measure * 7 + beat * 1 + time, quarter);
    playSineEnv(A3,       measure * 7 + beat * 2 + time, quarter);
    playSineEnv(E3,       measure * 7 + beat * 3 + time, quarter);
  }

  void part4_left_hand(float time){
    playSineEnv(D3,       measure * 0 + beat * 0 + time, whole);
    playSineEnv(A3,       measure * 0 + beat * 1 + time, quarter);
    playSineEnv(D4,       measure * 0 + beat * 2 + time, quarter);
    playSineEnv(A3,       measure * 0 + beat * 3 + time, quarter);
   
    playSineEnv(A2,       measure * 1 + beat * 0 + time, whole);
    playSineEnv(E3,       measure * 1 + beat * 1 + time, quarter);
    playSineEnv(A3,       measure * 1 + beat * 2 + time, quarter);
    playSineEnv(E3,       measure * 1 + beat * 3 + time, quarter);
    
    playSineEnv(B2,       measure * 2 + beat * 0 + time, whole);
    playSineEnv(Fsharp3,  measure * 2 + beat * 1 + time, quarter);
    playSineEnv(B3,       measure * 2 + beat * 2 + time, quarter);
    playSineEnv(Fsharp3,  measure * 2 + beat * 3 + time, quarter);
  
    playSineEnv(Fsharp2,  measure * 3 + beat * 0 + time, whole);
    playSineEnv(Csharp3,  measure * 3 + beat * 1 + time, quarter);
    playSineEnv(Fsharp3,  measure * 3 + beat * 2 + time, quarter);
    playSineEnv(Csharp3,  measure * 3 + beat * 3 + time, quarter);
    
    playSineEnv(G2,       measure * 4 + beat * 0 + time, whole);
    playSineEnv(D3,       measure * 4 + beat * 1 + time, quarter);
    playSineEnv(G3,       measure * 4 + beat * 2 + time, quarter);
    playSineEnv(D3,       measure * 4 + beat * 3 + time, quarter);
   
    playSineEnv(D2,       measure * 5 + beat * 0 + time, whole);
    playSineEnv(A2,       measure * 5 + beat * 1 + time, quarter);
    playSineEnv(D3,       measure * 5 + beat * 2 + time, quarter);
    playSineEnv(A2,       measure * 5 + beat * 3 + time, quarter);
    
    playSineEnv(G2,       measure * 6 + beat * 0 + time, whole);
    playSineEnv(D3,       measure * 6 + beat * 1 + time, quarter);
    playSineEnv(G3,       measure * 6 + beat * 2 + time, quarter);
    playSineEnv(D3,       measure * 6 + beat * 3 + time, quarter);
  
    playSineEnv(A2,       measure * 7 + beat * 0 + time, whole);
    playSineEnv(E3,       measure * 7 + beat * 1 + time, quarter);
    playSineEnv(A3,       measure * 7 + beat * 2 + time, quarter);
    playSineEnv(E3,       measure * 7 + beat * 3 + time, quarter);

  }

  void part4_right_hand(float time){
    playSineEnv(D5,       measure * 0 + beat * 0 + time, quarter);
    playSineEnv(Csharp5,  measure * 0 + beat * 1 + time, quarter);
    playSineEnv(D5,       measure * 0 + beat * 2 + time, half);
    playSineEnv(Csharp5,  measure * 1 + beat * 0 + time, whole);
    playSineEnv(D5,       measure * 2 + beat * 0 + time, half+quarter);
    playSineEnv(Fsharp5,  measure * 2 + beat * 3 + time, quarter);
    playSineEnv(A5,       measure * 3 + beat * 0 + time, half+quarter);
    playSineEnv(B5,       measure * 3 + beat * 3 + time, quarter);
    playSineEnv(G5,       measure * 4 + beat * 0 + time, quarter);
    playSineEnv(Fsharp5,  measure * 4 + beat * 1 + time, quarter);
    playSineEnv(E5,       measure * 4 + beat * 2 + time, quarter);
    playSineEnv(G5,       measure * 4 + beat * 3 + time, quarter);
    playSineEnv(Fsharp5,  measure * 5 + beat * 0 + time, quarter);
    playSineEnv(E5,       measure * 5 + beat * 1 + time, quarter);
    playSineEnv(D5,       measure * 5 + beat * 2 + time, quarter);
    playSineEnv(Csharp5,  measure * 5 + beat * 3 + time, quarter);
    playSineEnv(B4,       measure * 6 + beat * 0 + time, quarter);
    playSineEnv(A4,       measure * 6 + beat * 1 + time, quarter);
    playSineEnv(G4,       measure * 6 + beat * 2 + time, quarter);
    playSineEnv(Fsharp4,  measure * 6 + beat * 3 + time, quarter);
    playSineEnv(E4,       measure * 7 + beat * 0 + time, quarter);
    playSineEnv(G4,       measure * 7 + beat * 1 + time, quarter);
    playSineEnv(Fsharp4,  measure * 7 + beat * 2 + time, quarter);
    playSineEnv(E4,       measure * 7 + beat * 3 + time, quarter);
  }

  void part5_right_hand(float time){
    playSineEnv(D4,       measure * 0 + beat * 0 + time, quarter);
    playSineEnv(E4,       measure * 0 + beat * 1 + time, quarter);
    playSineEnv(Fsharp4,  measure * 0 + beat * 2 + time, quarter);
    playSineEnv(G4,       measure * 0 + beat * 3 + time, quarter);
    playSineEnv(A4,       measure * 1 + beat * 0 + time, quarter);
    playSineEnv(E4,       measure * 1 + beat * 1 + time, quarter);
    playSineEnv(A4,       measure * 1 + beat * 2 + time, quarter);
    playSineEnv(G4,       measure * 1 + beat * 3 + time, quarter);
    playSineEnv(Fsharp4,  measure * 2 + beat * 0 + time, quarter);
    playSineEnv(B4,       measure * 2 + beat * 1 + time, quarter);
    playSineEnv(A4,       measure * 2 + beat * 2 + time, quarter);
    playSineEnv(G4,       measure * 2 + beat * 3 + time, quarter);
    playSineEnv(A4,       measure * 3 + beat * 0 + time, quarter);
    playSineEnv(G4,       measure * 3 + beat * 1 + time, quarter);
    playSineEnv(Fsharp4,  measure * 3 + beat * 2 + time, quarter);
    playSineEnv(E4,       measure * 3 + beat * 3 + time, quarter);

    playSineEnv(D4,       measure * 4 + beat * 0 + time, half);
    playSineEnv(B4,       measure * 4 + beat * 2 + time, quarter);
    playSineEnv(Csharp5,  measure * 4 + beat * 3 + time, quarter);
    playSineEnv(D5,       measure * 5 + beat * 0 + time, quarter);
    playSineEnv(Csharp5,  measure * 5 + beat * 1 + time, quarter);
    playSineEnv(B4,       measure * 5 + beat * 2 + time, quarter);
    playSineEnv(A4,       measure * 5 + beat * 3 + time, quarter);
    playSineEnv(B4,       measure * 6 + beat * 0 + time, quarter);
    playSineEnv(A4,       measure * 6 + beat * 1 + time, quarter);
    playSineEnv(B4,       measure * 6 + beat * 2 + time, quarter);
    playSineEnv(D5,       measure * 6 + beat * 3 + time, quarter);
    playSineEnv(D5,       measure * 7 + beat * 0 + time, half);
    playSineEnv(Csharp5,  measure * 7 + beat * 2 + time, quarter);
    playSineEnv(D5,       measure * 7 + beat * 3 + time, quarter);
  }

  void part6_right_hand(float time){
    playSineEnv(A5,       measure * 0 + beat * 0 + time, quarter);
    playSineEnv(Fsharp5,  measure * 0 + beat * 1 + time, eigth);
    playSineEnv(G5,       measure * 0 + beat * 1.5 + time, eigth);
    playSineEnv(A5,       measure * 0 + beat * 2 + time, quarter);
    playSineEnv(Fsharp5,  measure * 0 + beat * 3 + time, eigth);
    playSineEnv(G5,       measure * 0 + beat * 3.5 + time, eigth);

    playSineEnv(A5,       measure * 1 + beat * 0 + time, eigth);
    playSineEnv(A4,       measure * 1 + beat * 0.5 + time, eigth);
    playSineEnv(B4,       measure * 1 + beat * 1 + time, eigth);
    playSineEnv(Csharp5,  measure * 1 + beat * 1.5 + time, eigth);
    playSineEnv(D5,       measure * 1 + beat * 2 + time, eigth);
    playSineEnv(E5,       measure * 1 + beat * 2.5 + time, eigth);
    playSineEnv(Fsharp5,  measure * 1 + beat * 3 + time, eigth);
    playSineEnv(G5,       measure * 1 + beat * 3.5 + time, eigth);

    playSineEnv(Fsharp5,  measure * 2 + beat * 0 + time, quarter);
    playSineEnv(D5,       measure * 2 + beat * 1 + time, eigth);
    playSineEnv(E5,       measure * 2 + beat * 1.5 + time, eigth);
    playSineEnv(Fsharp5,  measure * 2 + beat * 2 + time, quarter);
    playSineEnv(Fsharp4,  measure * 2 + beat * 3 + time, eigth);
    playSineEnv(G4,       measure * 2 + beat * 3.5 + time, eigth);

    playSineEnv(A4,       measure * 3 + beat * 0 + time, eigth);
    playSineEnv(B4,       measure * 3 + beat * 0.5 + time, eigth);
    playSineEnv(A4,       measure * 3 + beat * 1 + time, eigth);
    playSineEnv(G4,       measure * 3 + beat * 1.5 + time, eigth);
    playSineEnv(A4,       measure * 3 + beat * 2 + time, eigth);
    playSineEnv(Fsharp4,  measure * 3 + beat * 2.5 + time, eigth);
    playSineEnv(G4,       measure * 3 + beat * 3 + time, eigth);
    playSineEnv(A4,       measure * 3 + beat * 3.5 + time, eigth);

    playSineEnv(G4,       measure * 4 + beat * 0 + time, quarter);
    playSineEnv(B4,       measure * 4 + beat * 1 + time, eigth);
    playSineEnv(A4,       measure * 4 + beat * 1.5 + time, eigth);
    playSineEnv(G4,       measure * 4 + beat * 2 + time, quarter);
    playSineEnv(Fsharp4,  measure * 4 + beat * 3 + time, eigth);
    playSineEnv(E4,       measure * 4 + beat * 3.5 + time, eigth);

    playSineEnv(Fsharp4,  measure * 5 + beat * 0 + time, eigth);
    playSineEnv(E4,       measure * 5 + beat * 0.5 + time, eigth);
    playSineEnv(D4,       measure * 5 + beat * 1 + time, eigth);
    playSineEnv(E4,       measure * 5 + beat * 1.5 + time, eigth);
    playSineEnv(Fsharp4,  measure * 5 + beat * 2 + time, eigth);
    playSineEnv(G4,       measure * 5 + beat * 2.5 + time, eigth);
    playSineEnv(A4,       measure * 5 + beat * 3 + time, eigth);
    playSineEnv(B4,       measure * 5 + beat * 3.5 + time, eigth);

    playSineEnv(G4,       measure * 6 + beat * 0 + time, quarter);
    playSineEnv(B4,       measure * 6 + beat * 1 + time, eigth);
    playSineEnv(A4,       measure * 6 + beat * 1.5 + time, eigth);
    playSineEnv(B4,       measure * 6 + beat * 2 + time, quarter);
    playSineEnv(Csharp5,  measure * 6 + beat * 3 + time, eigth);
    playSineEnv(D5,       measure * 6 + beat * 3.5 + time, eigth);

    playSineEnv(Csharp5,  measure * 7 + beat * 0 + time, eigth);
    playSineEnv(A4,       measure * 7 + beat * 0.5 + time, eigth);
    playSineEnv(B4,       measure * 7 + beat * 1 + time, eigth);
    playSineEnv(Csharp5,  measure * 7 + beat * 1.5 + time, eigth);
    playSineEnv(D5,       measure * 7 + beat * 2 + time, eigth);
    playSineEnv(E5,       measure * 7 + beat * 2.5 + time, eigth);
    playSineEnv(Fsharp5,  measure * 7 + beat * 3 + time, eigth);
    playSineEnv(G5,       measure * 7 + beat * 3.5 + time, eigth);
  }

  

  void playSong(){
    part1(0);
    part2(0 + 8*measure);
    part3(0 + 16*measure);
    part4_left_hand(0 + 24*measure);
    part4_right_hand(0 + 24*measure);
    part4_left_hand(0 + 32*measure);
    part5_right_hand(0 + 32*measure);
    part4_left_hand(0 + 40*measure);
    part6_right_hand(0 + 40*measure);
    part4_left_hand(0 + 48*measure);
    part6_right_hand(0 + 48*measure);
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