/*
    main.cpp

    Copyright 2009-2011, Alan Calvert
    Copyright 2014-2019, Will Godfrey & others

    This file is part of yoshimi, which is free software: you can
    redistribute it and/or modify it under the terms of the GNU General
    Public License as published by the Free Software Foundation, either
    version 2 of the License, or (at your option) any later version.

    yoshimi is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with yoshimi.  If not, see <http://www.gnu.org/licenses/>.

    Modified February 2019
*/

// approx timeout in seconds.
#define SPLASH_TIME 3

#include <sys/mman.h>
#include <iostream>
#include <stdio.h>
#include <sys/types.h>
#include <termios.h>
#include <time.h>


using namespace std;

#include "Misc/Config.h"
#include "Misc/SynthEngine.h"
#include "MusicIO/MusicClient.h"
#include <map>
#include <list>
#include <pthread.h>
#include <semaphore.h>
#include <cstdio>
#include <unistd.h>

#ifdef GUI_FLTK
    #include "MasterUI.h"
    #include "UI/MiscGui.h"
    #include <FL/Fl.H>
    #include <FL/Fl_Window.H>
    #include <FL/Fl_PNG_Image.H>
    #include "Misc/Splash.h"
#endif

#include <readline/readline.h>
#include <readline/history.h>
#include <Interface/CmdInterface.h>

CmdInterface commandInt;

extern map<SynthEngine *, MusicClient *> synthInstances;
extern SynthEngine *firstSynth;
extern int startInstance;

void mainRegisterAudioPort(SynthEngine *s, int portnum);
int mainCreateNewInstance(unsigned int forceId, bool loadState);

Config *firstRuntime = NULL;
static int globalArgc = 0;
static char **globalArgv = NULL;
bool bShowGui = true;
bool bShowCmdLine = true;
bool splashSet = true;
time_t old_father_time, here_and_now;

//Andrew Deryabin: signal handling moved to main from Config Runtime
//It's only suitable for single instance app support
static struct sigaction yoshimiSigAction;

void yoshimiSigHandler(int sig)
{
    switch (sig)
    {
        case SIGINT:
        case SIGHUP:
        case SIGTERM:
        case SIGQUIT:
            firstRuntime->setInterruptActive();
            break;

        case SIGUSR1:
            firstRuntime->setLadi1Active();
            sigaction(SIGUSR1, &yoshimiSigAction, NULL);
            break;

        case SIGUSR2: // start next instance
            mainCreateNewInstance(0, true);
            sigaction(SIGUSR2, &yoshimiSigAction, NULL);
            break;

        default:
            break;
    }
}

static void *mainGuiThread(void *arg)
{
#ifdef GUI_FLTK
    Fl::lock();
#endif

    sem_post((sem_t *)arg);

    map<SynthEngine *, MusicClient *>::iterator it;

#ifdef GUI_FLTK
    const int textHeight = 15;
    const int textY = 10;
    const unsigned char lred = 0xd7;
    const unsigned char lgreen = 0xf7;
    const unsigned char lblue = 0xff;

    Fl_PNG_Image pix("splash_screen_png", splashPngData, splashPngLength);
    Fl_Window winSplash(splashWidth, splashHeight, "yoshimi splash screen");
    Fl_Box box(0, 0, splashWidth,splashHeight);
    box.image(pix);
    string startup = YOSHIMI_VERSION;
    startup = "V " + startup;
    Fl_Box boxLb(0, splashHeight - textY - textHeight, splashWidth, textHeight, startup.c_str());
    boxLb.box(FL_NO_BOX);
    boxLb.align(FL_ALIGN_CENTER);
    boxLb.labelsize(textHeight);
    boxLb.labeltype(FL_NORMAL_LABEL);
    boxLb.labelcolor(fl_rgb_color(lred, lgreen, lblue));
    boxLb.labelfont(FL_HELVETICA | FL_BOLD);
    winSplash.border(false);
    if (splashSet && bShowGui && firstRuntime->showSplash)
    {
        winSplash.position((Fl::w() - winSplash.w()) / 2, (Fl::h() - winSplash.h()) / 2);
    }
    else
        splashSet = false;
    do
    {
            usleep(33333);
    }
#endif
    while (firstSynth == NULL); // just wait

#ifdef GUI_FLTK
    GuiThreadMsg::sendMessage(firstSynth, GuiThreadMsg::NewSynthEngine, 0);
#endif
    if (firstRuntime->autoInstance)
    {
        for (int i = 1; i < 32; ++i)
        {
            if ((firstRuntime->activeInstance >> i) & 1)
                mainCreateNewInstance(i, true);
        }
    }
    while (firstRuntime->runSynth)
    {
        if (firstSynth->getUniqueId() == 0)
        {
            firstRuntime->signalCheck();
        }

        for (it = synthInstances.begin(); it != synthInstances.end(); ++it)
        {
            SynthEngine *_synth = it->first;
            MusicClient *_client = it->second;
            if (!_synth->getRuntime().runSynth && _synth->getUniqueId() > 0)
            {
                if (_synth->getRuntime().configChanged)
                {
                    size_t tmpRoot = _synth->ReadBankRoot();
                    size_t tmpBank = _synth->ReadBank();
                    _synth->getRuntime().loadConfig(); // restore old settings
                    _synth->RootBank(tmpRoot, tmpBank); // but keep current root and bank
                }
                _synth->getRuntime().saveConfig();
                unsigned int instanceID =  _synth->getUniqueId();
                if (_client)
                {
                    _client->Close();
                    delete _client;
                }

                if (_synth)
                {
                    int instancebit = (1 << instanceID);
                    if (_synth->getRuntime().activeInstance & instancebit)
                        _synth->getRuntime().activeInstance -= instancebit;
                    _synth->saveBanks();
                    _synth->getRuntime().flushLog();
                    delete _synth;
                }

                synthInstances.erase(it);
                cout << "\nStopped " << instanceID << "\n";
                break;
            }
#ifdef GUI_FLTK
            if (bShowGui)
            {
                for (int i = 0; !_synth->getRuntime().LogList.empty() && i < 5; ++i)
                {
                    MasterUI *guiMaster = _synth->getGuiMaster(false);
                    if (guiMaster)
                    { guiMaster->Log(_synth->getRuntime().LogList.front());
                        _synth->getRuntime().LogList.pop_front();
                    }
                }
            }
#endif
            if (_synth == firstSynth)
            {
                int testInstance = startInstance;
                if (testInstance > 0xff)
                    startInstance = mainCreateNewInstance(testInstance & 0xff, false);
            }
        }

        // where all the action is ...
#ifdef GUI_FLTK
        if (bShowGui)
        {
            if (splashSet)
            {
                winSplash.show();
                usleep(1000);
                if(time(&here_and_now) < 0) // no time?
                    here_and_now = old_father_time + SPLASH_TIME;
                if ((here_and_now - old_father_time) >= SPLASH_TIME)
                {
                    splashSet = false;
                    winSplash.hide();
                }
            }
            Fl::wait(0.033333);
            GuiThreadMsg::processGuiMessages();
        }
        else
#endif
            usleep(33333);
    }
    if (firstRuntime->configChanged && (bShowGui | bShowCmdLine)) // don't want this if no cli or gui
    {
        size_t tmpRoot = firstSynth->ReadBankRoot();
        size_t tmpBank = firstSynth->ReadBank();
        firstRuntime->loadConfig(); // restore old settings
        firstSynth->RootBank(tmpRoot, tmpBank); // but keep current root and bank
    }

    firstRuntime->saveConfig();
    firstSynth->saveHistory();
    firstSynth->saveBanks();
    return NULL;
}

int mainCreateNewInstance(unsigned int forceId, bool loadState)
{
    MusicClient *musicClient = NULL;
    unsigned int instanceID;
    SynthEngine *synth = new SynthEngine(globalArgc, globalArgv, false, forceId);
    if (!synth->getRuntime().isRuntimeSetupCompleted())
        goto bail_out;
    instanceID = synth->getUniqueId();
    if (!synth)
    {
        std::cerr << "Failed to allocate SynthEngine" << std::endl;
        goto bail_out;
    }

    if (!(musicClient = MusicClient::newMusicClient(synth)))
    {
        synth->getRuntime().Log("Failed to instantiate MusicClient");
        goto bail_out;
    }

    if (!synth->Init(musicClient->getSamplerate(), musicClient->getBuffersize()))
    {
        synth->getRuntime().Log("SynthEngine init failed");
        goto bail_out;
    }

    if (!musicClient->Start())
    {
        synth->getRuntime().Log("Failed to start MusicIO");
        goto bail_out;
    }
     // TODO sort this out properly!
     // it works, but is clunky :(
    loadState = synth->getRuntime().loadDefaultState;
    if (loadState)
    {
        string name = synth->getRuntime().defaultStateName;
        if (instanceID > 0)
            name = name + "-" + to_string(forceId);
        synth->loadStateAndUpdate(name);
    }
#ifdef GUI_FLTK
    if (synth->getRuntime().showGui)
    {
        synth->setWindowTitle(musicClient->midiClientName());
        if(firstSynth != NULL) //FLTK is not ready yet - send this message later for first synth
        {
            GuiThreadMsg::sendMessage(synth, GuiThreadMsg::NewSynthEngine, 0);
        }
        if (synth->getRuntime().audioEngine < 1)
            fl_alert("Yoshimi can't find an available sound system. Running with no Audio");
        if (synth->getRuntime().midiEngine < 1)
            fl_alert("Yoshimi can't find an input system. Running with no MIDI");
    }
#endif
    synth->getRuntime().StartupReport(musicClient->midiClientName());
    synth->Unmute();

    if (instanceID == 0)
        cout << "\nYay! We're up and running :-)\n";
    else
    {
        cout << "\nStarted "<< instanceID << "\n";
        // following copied here for other instances
        synth->installBanks();
    }
    synthInstances.insert(std::make_pair(synth, musicClient));
    //register jack ports for enabled parts
    for (int npart = 0; npart < NUM_MIDI_PARTS; ++npart)
    {
        if (synth->partonoffRead(npart))
            mainRegisterAudioPort(synth, npart);
    }
    synth->getRuntime().activeInstance |= (1 << instanceID);
    return instanceID;

bail_out:
    synth->getRuntime().runSynth = false;
    synth->getRuntime().Log("Bail: Yoshimi stages a strategic retreat :-(");
    if (musicClient)
    {
        musicClient->Close();
        delete musicClient;
    }
    if (synth)
    {
        synth->getRuntime().flushLog();
        delete synth;
    }

    return -1;
}

void *commandThread(void *arg = NULL) // silence warning
{
    commandInt.cmdIfaceCommandLoop();
    return 0;
}

int main(int argc, char *argv[])
{
    char pidline[256];
    memset(&pidline, 0, 255);
    // test for *exact* name and only the oldest occurrance
    FILE *fp = popen("pgrep -o -x yoshimi", "r");
    fgets(pidline,255,fp);
    //cout << "> " << pidline << " <" << endl;
    pclose(fp);
    int firstpid = stoi(pidline);
    // we try to failsafe if no valid PID is returned
    if (firstpid > 1 && firstpid != getpid())
    {
        //cout << "got it" << endl;
        kill(firstpid, SIGUSR2);
        return 0;
    }

    time(&old_father_time);
    here_and_now = old_father_time;
    struct termios  oldTerm;
    tcgetattr(0, &oldTerm);

    cout << "Yoshimi " << YOSHIMI_VERSION << " is starting" << endl; // guaranteed start message
    globalArgc = argc;
    globalArgv = argv;
    bool bExitSuccess = false;
    map<SynthEngine *, MusicClient *>::iterator it;
#ifdef GUI_FLTK
    bool guiStarted = false;
#endif
    pthread_t thr;
    pthread_attr_t attr;
    sem_t semGui;

    if (mainCreateNewInstance(0, false) == -1)
    {
        goto bail_out;
    }

    it = synthInstances.begin();
    firstRuntime = &it->first->getRuntime();
    firstSynth = it->first;
    bShowGui = firstRuntime->showGui;
    bShowCmdLine = firstRuntime->showCLI;

    if (firstRuntime->oldConfig)
    {

        cout << "\nExisting config older than " << MIN_CONFIG_MAJOR << "." << MIN_CONFIG_MINOR << "\nCheck settings, save and restart.\n"<< endl;
    }
    if(sem_init(&semGui, 0, 0) == 0)
    {
        if (pthread_attr_init(&attr) == 0)
        {
            if (pthread_create(&thr, &attr, mainGuiThread, (void *)&semGui) == 0)
            {
#ifdef GUI_FLTK
                guiStarted = true;
#endif
            }
            pthread_attr_destroy(&attr);
        }
    }
#ifdef GUI_FLTK
    if (!guiStarted)
    {
        cout << "Yoshimi can't start main gui loop!" << endl;
        goto bail_out;
    }
    sem_wait(&semGui);
    sem_destroy(&semGui);
#endif
    memset(&yoshimiSigAction, 0, sizeof(yoshimiSigAction));
    yoshimiSigAction.sa_handler = yoshimiSigHandler;
    if (sigaction(SIGUSR1, &yoshimiSigAction, NULL))
        firstRuntime->Log("Setting SIGUSR1 handler failed");
    if (sigaction(SIGUSR2, &yoshimiSigAction, NULL))
        firstRuntime->Log("Setting SIGUSR2 handler failed");
    if (sigaction(SIGINT, &yoshimiSigAction, NULL))
        firstRuntime->Log("Setting SIGINT handler failed");
    if (sigaction(SIGHUP, &yoshimiSigAction, NULL))
        firstRuntime->Log("Setting SIGHUP handler failed");
    if (sigaction(SIGTERM, &yoshimiSigAction, NULL))
        firstRuntime->Log("Setting SIGTERM handler failed");
    if (sigaction(SIGQUIT, &yoshimiSigAction, NULL))
        firstRuntime->Log("Setting SIGQUIT handler failed");
    // following moved here for faster first synth startup
    firstSynth->loadHistory();
    firstSynth->installBanks();

    //create command line processing thread
    pthread_t cmdThr;

    if(bShowCmdLine)
    {
        if (pthread_attr_init(&attr) == 0)
        {
            if (pthread_create(&cmdThr, &attr, commandThread, (void *)firstSynth) == 0)
            {

            }
            pthread_attr_destroy(&attr);
        }
    }

    void *ret;
    pthread_join(thr, &ret);
    if(ret == (void *)1)
    {
        goto bail_out;
    }
    cout << "\nGoodbye - Play again soon?\n";
    bExitSuccess = true;

bail_out:
    for (it = synthInstances.begin(); it != synthInstances.end(); ++it)
    {
        SynthEngine *_synth = it->first;
        MusicClient *_client = it->second;
        _synth->getRuntime().runSynth = false;
        if(!bExitSuccess)
        {
            _synth->getRuntime().Log("Bail: Yoshimi stages a strategic retreat :-(");
        }

        if (_client)
        {
            _client->Close();
            delete _client;
        }

        if (_synth)
        {
            _synth->getRuntime().flushLog();
            delete _synth;
        }
    }
    if(bShowCmdLine)
        tcsetattr(0, TCSANOW, &oldTerm);
    munlockall(); // just to be sure
    if (bExitSuccess)
        exit(EXIT_SUCCESS);
    else
        exit(EXIT_FAILURE);
}

void mainRegisterAudioPort(SynthEngine *s, int portnum)
{
    if (s && (portnum < NUM_MIDI_PARTS) && (portnum >= 0))
    {
        map<SynthEngine *, MusicClient *>::iterator it = synthInstances.find(s);
        if (it != synthInstances.end())
        {
            it->second->registerAudioPort(portnum);
        }
    }
}
