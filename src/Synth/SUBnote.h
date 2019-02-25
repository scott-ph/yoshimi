/*
    SUBnote.h - The subtractive synthesizer

    Original ZynAddSubFX author Nasca Octavian Paul
    Copyright (C) 2002-2005 Nasca Octavian Paul
    Copyright 2009-2010 Alan Calvert
    Copyright 2014-2017 Will Godfrey & others

    This file is part of yoshimi, which is free software: you can redistribute
    it and/or modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    yoshimi is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.   See the GNU General Public License (version 2 or
    later) for more details.

    You should have received a copy of the GNU General Public License along with
    yoshimi; if not, write to the Free Software Foundation, Inc., 51 Franklin
    Street, Fifth Floor, Boston, MA  02110-1301, USA.

    This file is a derivative of a ZynAddSubFX original
    Modified September 2017
*/

#ifndef SUB_NOTE_H
#define SUB_NOTE_H

#include "Misc/SynthHelper.h"
#include "Synth/LegatoTypes.h"

class SUBnoteParameters;
class Controller;
class Envelope;
class Filter;

class SynthEngine;

class SUBnote : private SynthHelper
{
    public:
        SUBnote(SUBnoteParameters *parameters, Controller *ctl_,
                float freq, float velocity, int portamento_,
                int midinote, bool besilent, SynthEngine *_synth);
        ~SUBnote();

        void SUBlegatonote(float freq, float velocity,
                           int portamento_, int midinote, bool externcall);

        int noteout(float *outl,float *outr); // note output, return 0 if the
                                              // note is finished
        void releasekey(void);
        bool finished(void) { return !NoteEnabled; }

        bool ready; // if I can get the sampledata

    private:
        void computecurrentparameters(void);
        void initparameters(float freq);
        void KillNote(void);
        void initfilterbank(void);

        SUBnoteParameters *pars;

        bool stereo;
        int pos[MAX_SUB_HARMONICS]; // chart of non-zero harmonic locations
        int numstages; // number of stages of filters
        int numharmonics; // number of harmonics (after the too higher hamonics are removed)
        int firstnumharmonics; // To keep track of the first note's numharmonics value, useful in legato mode.
        int start; // how the harmonics start
        float basefreq;
        float BendAdjust;
        float OffsetHz;
        float randpanL;
        float randpanR;

        Envelope *AmpEnvelope;
        Envelope *FreqEnvelope;
        Envelope *BandWidthEnvelope;

        Filter *GlobalFilterL,*GlobalFilterR;

        Envelope *GlobalFilterEnvelope;

        // internal values
        bool NoteEnabled;
        int firsttick;
        int portamento;
        float volume;
        float oldamplitude;
        float newamplitude;

        float GlobalFilterCenterPitch; // octaves
        float GlobalFilterFreqTracking;

        struct bpfilter {
            float freq;
            float bw;
            float amp;   // filter parameters
            float a1;
            float a2;
            float b0;
            float b2;    // filter coefs. b1=0
            float xn1;
            float xn2;
            float yn1;
            float yn2;   // filter internal values
        };

        void initfilter(bpfilter &filter, float freq, float bw, float amp, float mag);
        float computerolloff(float freq);
        void computefiltercoefs(bpfilter &filter, float freq, float bw, float gain);
        void filter(bpfilter &filter, float *smps);
        void filterVarRun(bpfilter &filter, float *smps);

        bpfilter *lfilter;
        bpfilter *rfilter;

        float overtone_rolloff[MAX_SUB_HARMONICS];
        float overtone_freq[MAX_SUB_HARMONICS];

        float *tmpsmp;
        float *tmprnd; // this is filled with random numbers

        Controller *ctl;
        int oldpitchwheel;
        int oldbandwidth;
        float globalfiltercenterq;

        // Legato vars
        struct {
            bool silent;
            float lastfreq;
            LegatoMsg msg;
            int decounter;
            struct {
                // Fade In/Out vars
                int length;
                float m;
                float step;
            } fade;

            struct {
                // Note parameters
                float freq;
                float vel;
                int portamento;
                int midinote;
            } param;
        } Legato;

        const float log_0_01;    // logf(0.01);
        const float log_0_001;   // logf(0.001);
        const float log_0_0001;  // logf(0.0001);
        const float log_0_00001; // logf(0.00001);

        SynthEngine *synth;
};

#endif
