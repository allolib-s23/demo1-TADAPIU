#include <cstdio> // for printing to stdout

#include "Gamma/Analysis.h"
#include "Gamma/Effects.h"
#include "Gamma/Envelope.h"
#include "Gamma/Oscillator.h"

#include "al/app/al_App.hpp"
#include "al/scene/al_PolySynth.hpp"
#include "al/scene/al_SynthSequencer.hpp"
#include "al/ui/al_ControlGUI.hpp"
#include "al/ui/al_Parameter.hpp"

using namespace al;
#include <iostream>
#include <vector>
using namespace std;

// This example shows how to use SynthVoice and SynthManager to create an audio
// visual synthesizer. In a class that inherits from SynthVoice you will
// define the synth's voice parameters and the sound and graphic generation
// processes in the onProcess() functions.

class SineEnv : public SynthVoice
{
public:
  // Unit generators
  gam::Pan<> mPan;
  // gam::Sine<> mOsc;
  gam::Sine<> mOsc1;
  gam::Sine<> mOsc3;
  gam::Sine<> mOsc5;
  gam::Sine<> mOsc7;
  gam::Sine<> mOsc9;
  gam::Sine<> mOsc11;
  gam::Sine<> mOsc13;
  gam::Sine<> mOsc15;
  gam::Sine<> mOsc17;
  gam::Saw<> mSaw1;
  gam::Saw<> mSaw3;
  gam::Saw<> mSaw5;
  gam::Buzz<> mBuzz1;
  gam::Impulse<> mImp1;
  gam::SineR<> mSR1;
  gam::SineD<> mSD1;
  gam::LFO<> mLFO1;
  gam::DWO<> mDWO1;
  // vector<gam::Sine<>*> mOsc_vect(5);
  // vector<gam::Sine<>> mOsc_vect(5);
  gam::Env<3> mAmpEnv;

  // Initialize voice. This function will only be called once per voice when
  // it is created. Voices will be reused if they are idle.
  void init() override
  {
    // Intialize envelope
    mAmpEnv.curve(10); // make segments lines
    // mAmpEnv.levels(0, 1, 0.8, 0.3);
    mAmpEnv.levels(0, 1, 0.8, 0);
    mAmpEnv.sustainPoint(2); // Make point 2 sustain until a release is issued

    // This is a quick way to create parameters for the voice. Trigger
    // parameters are meant to be set only when the voice starts, i.e. they
    // are expected to be constant within a voice instance. (You can actually
    // change them while you are prototyping, but their changes will only be
    // stored and aplied when a note is triggered.)

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
    mOsc3.freq(2*getInternalParameterValue("frequency"));
    mOsc5.freq(3*getInternalParameterValue("frequency"));
    mOsc7.freq(4*getInternalParameterValue("frequency"));
    mOsc9.freq(5*getInternalParameterValue("frequency"));
    mOsc11.freq(6*getInternalParameterValue("frequency"));
    mOsc13.freq(7*getInternalParameterValue("frequency"));
    mOsc15.freq(8*getInternalParameterValue("frequency"));
    mOsc17.freq(17*getInternalParameterValue("frequency"));
    mSaw1.freq(getInternalParameterValue("frequency"));
    mSaw3.freq(3*getInternalParameterValue("frequency"));
    mSaw5.freq(2*getInternalParameterValue("frequency"));
    mBuzz1.freq(getInternalParameterValue("frequency"));
    mImp1.freq(getInternalParameterValue("frequency"));
    mSR1.freq(getInternalParameterValue("frequency"));
    mSD1.freq(getInternalParameterValue("frequency"));
    mLFO1.freq(getInternalParameterValue("frequency"));
    mDWO1.freq(getInternalParameterValue("frequency"));

    
    /*for(int i=0; i<5; i++){
      gam::Sine<> mOsc;
      mOsc.freq((i*2+1)*getInternalParameterValue("frequency"));
      mOsc_vect.push_back(&mOsc);
    }*/

    /*gam::Sine<> mOsc;
    for(int i=0; i<5; i++){
      mOsc_vect.push_back(mOsc);
    }

    for(int i=0; i<5; i++){
      mOsc_vect[i].freq((i*2+1)*getInternalParameterValue("frequency"));
    }*/

    /*for(int i=0; i<5; i++){
      mOsc_vect[i]->freq((i*2+1)*getInternalParameterValue("frequency"));
    }*/

    

    mAmpEnv.lengths()[0] = getInternalParameterValue("attackTime");
    mAmpEnv.lengths()[2] = getInternalParameterValue("releaseTime");
    mPan.pos(getInternalParameterValue("pan"));
    while (io())
    {
      /*float s1 = mOsc1() * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc3() * (1.0/3.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc5() * (1.0/5.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc7() * (1.0/7.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc9() * (1.0/9.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc11() * (1.0/11.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc13() * (1.0/13.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc15() * (1.0/15.0) * mAmpEnv() * getInternalParameterValue("amplitude")
               + mOsc17() * (1.0/17.0) * mAmpEnv() * getInternalParameterValue("amplitude");*/
      
      float s1 = mSaw1() * (1.0) * mAmpEnv() * getInternalParameterValue("amplitude")
              // + mSaw3() * (1.0/6.0) * mAmpEnv() * getInternalParameterValue("amplitude")
                + mSaw5() * (1.0/2.0) * mAmpEnv() * getInternalParameterValue("amplitude");
              + mOsc1() * (1.0/2.0) * mAmpEnv() * getInternalParameterValue("amplitude");

      /*float s1 = mOsc1() * (1.0/6.0)* mAmpEnv() * getInternalParameterValue("amplitude")
                + mOsc3() * (1.0/6.0)* mAmpEnv() * getInternalParameterValue("amplitude")
                + mOsc5() * (1.0/6.0)* mAmpEnv() * getInternalParameterValue("amplitude")
                + mOsc7() * (1.0/6.0)* mAmpEnv() * getInternalParameterValue("amplitude")
                + mOsc9() * (1.0/6.0)* mAmpEnv() * getInternalParameterValue("amplitude")
                + mOsc11() * (1.0/6.0)* mAmpEnv() * getInternalParameterValue("amplitude")
                + mOsc13() * (1.0/6.0)* mAmpEnv() * getInternalParameterValue("amplitude")
                + mOsc15() * (1.0/6.0)* mAmpEnv() * getInternalParameterValue("amplitude");*/
      
      //float s1 = mDWO1() * mAmpEnv() * getInternalParameterValue("amplitude");

      /*int k = 0;
      for(vector<gam::Sine<>*>::iterator i = mOsc_vect.begin();  i != mOsc_vect.end();  ++i ){
        s1 += (*i) * (1.0/(k*2+1.0)) * mAmpEnv() * getInternalParameterValue("amplitude");
        k++;
      }*/

      /*for(int i=0; i<5; i++){
        s1 += mOsc_vect[i] * (1.0/(i*2+1.0)) * mAmpEnv() * getInternalParameterValue("amplitude");
      }*/
      float s2;
      mPan(s1, s1, s2);
      io.out(0) += s1;
      io.out(1) += s2;
    }
    // We need to let the synth know that this voice is done
    // by calling the free(). This takes the voice out of the
    // rendering chain
    if (mAmpEnv.done()){
      /*for (vector<gam::Sine<>*>::iterator pObj = mOsc_vect.begin(); pObj != mOsc_vect.end(); ++pObj) {
        delete *pObj;
      }*/
      free();
    }
  }

  // The graphics processing function
  void onProcess(Graphics &g) override
  {
    // empty if there are no graphics to draw
  }

  // The triggering functions just need to tell the envelope to start or release
  // The audio processing function checks when the envelope is done to remove
  // the voice from the processing chain.
  void onTriggerOn() override { mAmpEnv.reset(); }

  void onTriggerOff() override { mAmpEnv.release(); }
};

// We make an app.
class MyApp : public App
{
public:
  // GUI manager for SineEnv voices
  // The name provided determines the name of the directory
  // where the presets and sequences are stored
  SynthGUIManager<SineEnv> synthManager{"SineEnv"};

  // This function is called right after the window is created
  // It provides a graphics context to initialize ParameterGUI
  // It's also a good place to put things that should
  // happen once at startup.
  void onCreate() override
  {
    navControl().active(false); // Disable navigation via keyboard, since we
                                // will be using keyboard for note triggering

    // Set sampling rate for Gamma objects from app's audio
    double frameRate = audioIO().framesPerSecond();
    cout << "frameRate=" << frameRate << endl;
    gam::sampleRate(audioIO().framesPerSecond());

    imguiInit();

    // Play example sequence. Comment this line to start from scratch
    synthManager.synthSequencer().playSequence("canon d.synthSequence");
    synthManager.synthRecorder().verbose(true);
  }

  // The audio callback function. Called when audio hardware requires data
  void onSound(AudioIOData &io) override
  {
    synthManager.render(io); // Render audio
  }

  void onAnimate(double dt) override
  {
    // The GUI is prepared here
    imguiBeginFrame();
    // Draw a window that contains the synth control panel
    synthManager.drawSynthControlPanel();
    imguiEndFrame();
  }

  // The graphics callback function.
  void onDraw(Graphics &g) override
  {
    g.clear();
    // Render the synth's graphics
    // synthManager.render(g);

    // GUI is drawn here
    imguiDraw();
  }

  // Whenever a key is pressed, this function is called
  bool onKeyDown(Keyboard const &k) override
  {
    // const float A4 = 432.f;
    const float A4 = 440.f;
    if (ParameterGUI::usingKeyboard())
    { // Ignore keys if GUI is using
      // keyboard
      return true;
    }
    if (k.shift())
    {
      // If shift pressed then keyboard sets preset
      int presetNumber = asciiToIndex(k.key());
      synthManager.recallPreset(presetNumber);
    }
    else
    {
      // Otherwise trigger note for polyphonic synth
      int midiNote = asciiToMIDI(k.key());
      if (midiNote > 0)
      {
        synthManager.voice()->setInternalParameterValue(
            "frequency", ::pow(2.f, (midiNote - 69.f) / 12.f) * A4);
        synthManager.triggerOn(midiNote);
      }
    }
    return true;
  }

  // Whenever a key is released this function is called
  bool onKeyUp(Keyboard const &k) override
  {
    int midiNote = asciiToMIDI(k.key());
    if (midiNote > 0)
    {
      synthManager.triggerOff(midiNote);
    }
    return true;
  }

  void onExit() override { imguiShutdown(); }
};

int main()
{
  // Create app instance
  MyApp app;

  // Set up audio
  app.configureAudio(48000., 512, 2, 0);

  app.start();

  return 0;
}