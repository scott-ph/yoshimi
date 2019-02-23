/*
    PresetsStore.cpp - Presets and Clipboard store

    Original ZynAddSubFX author Nasca Octavian Paul
    Copyright (C) 2002-2005 Nasca Octavian Paul
    Copyright 2009-2010, Alan Calvert
    Copyright 2017 Will Godfrey

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

    This file is a derivative of a ZynAddSubFX original.

    Modified September 2017
*/

#include <dirent.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>

#include "Misc/XMLwrapper.h"
#include "Params/PresetsStore.h"
#include "Misc/SynthEngine.h"

PresetsStore::_clipboard PresetsStore::clipboard;

PresetsStore::PresetsStore(SynthEngine *_synth) :
    preset_extension(".xpz"),
    synth(_synth)
{
    clipboard.data = 0;
    clipboard.type.clear();

    for (int i = 0; i < MAX_PRESETS; ++i)
    {
        presets[i].file.clear();
        presets[i].name.clear();
    }
}


PresetsStore::~PresetsStore()
{
    char *_data = reinterpret_cast<char *>(clipboard.data.exchange(0));
    free(_data);
    clearpresets();
}


// Clipboard management
void PresetsStore::copyclipboard(XMLwrapper *xml, string type)
{
    clipboard.type = type;
    uintptr_t newdata = reinterpret_cast<uintptr_t>(xml->getXMLdata());
    uintptr_t olddata = clipboard.data.exchange(newdata);
    free(reinterpret_cast<char *>(olddata));
}


bool PresetsStore::pasteclipboard(XMLwrapper *xml)
{
    uintptr_t empty = 0;
    uintptr_t udata = clipboard.data.exchange(0);
    char *data = reinterpret_cast<char *>(udata);
    if (data != NULL)
    {
        xml->putXMLdata(data);
        if (!clipboard.data.compare_exchange_strong(empty, udata))
            free(data);
        return true;
    }
    return false;
}


bool PresetsStore::checkclipboardtype(string type)
{
    // makes LFO's compatible
    if (type.find("Plfo") != string::npos
        && clipboard.type.find("Plfo") != string::npos)
        return true;
    return (!type.compare(clipboard.type));
}


void PresetsStore::clearpresets(void)
{
    for (int i = 0; i < MAX_PRESETS; ++i)
    {
        presets[i].file.clear();
        presets[i].name.clear();
    }
}


void PresetsStore::rescanforpresets(string type)
{
    for (int i = 0; i < MAX_PRESETS; ++i)
    {
        presets[i].file.clear();
        presets[i].name.clear();
    }
    int presetk = 0;
    string ftype = "." + type + preset_extension;

    for (int i = 0; i < MAX_PRESETS; ++i)
    {
        if (synth->getRuntime().presetsDirlist[i].empty())
            continue;
        string dirname = synth->getRuntime().presetsDirlist[i];
        DIR *dir = opendir(dirname.c_str());
        if (dir == NULL)
            continue;
        struct dirent *fn;
        while ((fn = readdir(dir)))
        {
            string filename = string(fn->d_name);
            if (filename.find(ftype) == string::npos)
                continue;
            if (dirname.at(dirname.size() - 1) != '/')
                dirname += "/";
            presets[presetk].file = dirname + filename;
            presets[presetk].name =
                filename.substr(0, filename.find(ftype));
            presetk++;
            if (presetk >= MAX_PRESETS)
                return;
        }
        closedir(dir);
    }
    // sort the presets
    bool check = true;
    while (check)
    {
        check = false;
        for (int j = 0; j < MAX_PRESETS - 1; ++j)
        {
            for (int i = j + 1; i < MAX_PRESETS; ++i)
            {
                if (presets[i].name.empty() || presets[j].name.empty())
                    continue;
                if (strcasecmp(presets[i].name.c_str(), presets[j].name.c_str()) < 0)
                {
                    presets[i].file.swap(presets[j].file);
                    presets[i].name.swap(presets[j].name);
                    check = true;
                }
            }
        }
    }
}


void PresetsStore::copypreset(XMLwrapper *xml, string type, string name)
{
    if (synth->getRuntime().presetsDirlist[0].empty())
        return;
    synth->getRuntime().xmlType = XML_PRESETS;
    synth->getRuntime().Log(name);
    string tmpfilename = name;
    legit_filename(tmpfilename);
    string dirname = synth->getRuntime().presetsDirlist[0];
    if (dirname.find_last_of("/") != (dirname.size() - 1))
        dirname += "/";
    xml->saveXMLfile(dirname + tmpfilename + "." + type + preset_extension);
}


bool PresetsStore::pastepreset(XMLwrapper *xml, int npreset)
{
    if (npreset >= MAX_PRESETS || npreset < 1)
        return false;
    npreset--;
    if (presets[npreset].file.empty())
        return false;
    return xml->loadXMLfile(presets[npreset].file);
}


void PresetsStore::deletepreset(int npreset)
{
    if (npreset >= MAX_PRESETS || npreset < 1)
        return;
    npreset--;
    if (!presets[npreset].file.empty())
        remove(presets[npreset].file.c_str());
}
