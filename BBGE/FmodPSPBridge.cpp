/*
Copyright (C) 2007, 2010 - Bit-Blot

This file is part of Aquaria.

Aquaria is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
// This file is adopted from FmodOpenALBridge.cpp to interface with the
// PSP audio engine.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "Core.h"

// Nothing OpenAL-specific in here, so just include it as is.
#include "FmodOpenALBridge.h"

#ifndef _DEBUG
//#define _DEBUG 1
#endif

/* for porting purposes... */
#ifndef STUBBED
#ifndef _DEBUG
#define STUBBED(x)
#else
#define STUBBED(x) { \
    static bool first_time = true; \
    if (first_time) { \
        first_time = false; \
        fprintf(stderr, "STUBBED: %s (%s, %s:%d)\n", x, __FUNCTION__, __FILE__, __LINE__); \
    } \
}
#endif
#endif

namespace FMOD {

// simply nasty.
#define PSPBRIDGE(cls,method,params,args) \
    FMOD_RESULT cls::method params { \
        if (!this) return FMOD_ERR_INTERNAL; \
        return ((PSP##cls *) this)->method args; \
    }

class PSPChannelGroup;
class PSPSound;

class PSPChannel
{
public:
    PSPChannel();
    FMOD_RESULT setVolume(const float _volume, const bool setstate=true);
    FMOD_RESULT setPaused(const bool _paused, const bool setstate=true);
    FMOD_RESULT setFrequency(const float _frequency);
    FMOD_RESULT setPriority(int _priority);
    FMOD_RESULT getPosition(unsigned int *position, FMOD_TIMEUNIT postype);
    FMOD_RESULT getVolume(float *_volume);
    FMOD_RESULT isPlaying(bool *isplaying);
    FMOD_RESULT setChannelGroup(ChannelGroup *channelgroup);
    FMOD_RESULT stop();
    void setGroupVolume(const float _volume);
    float getFinalVolume();
    void setSourceName(const unsigned int _sid) { sid = _sid; }
    unsigned int getSourceName() const { return sid; }
    void update();
    void reacquire();
    bool isInUse() const { return inuse; }
    void setSound(PSPSound *sound);

private:
    unsigned int sid;  // source id.
    float groupvolume;
    float volume;
    bool paused;
    int priority;
    float frequency;
    PSPChannelGroup *group;
    PSPSound *sound;
    bool inuse;
};


class PSPChannelGroup
{
public:
    PSPChannelGroup(const char *_name);
    ~PSPChannelGroup();
    FMOD_RESULT stop();
    FMOD_RESULT addDSP(DSP *dsp, DSPConnection **connection);
    FMOD_RESULT getPaused(bool *_paused);
    FMOD_RESULT setPaused(const bool _paused);
    FMOD_RESULT getVolume(float *_volume);
    FMOD_RESULT setVolume(const float _volume);
    bool attachChannel(PSPChannel *channel);
    void detachChannel(PSPChannel *channel);

private:
    const char *name;
    bool paused;
    int channel_count;
    PSPChannel **channels;
    float volume;
};


// FMOD::Channel implementation...

PSPChannel::PSPChannel()
    : sid(0)
    , groupvolume(1.0f)
    , volume(1.0f)
    , paused(false)
    , priority(0)
    , frequency(1.0f)
    , group(NULL)
    , sound(NULL)
    , inuse(false)
{
}

void PSPChannel::reacquire()
{
    assert(!inuse);
    inuse = true;
    volume = 1.0f;
    paused = true;
    priority = 0;
    frequency = 1.0f;
    sound = NULL;
}

void PSPChannel::setGroupVolume(const float _volume)
{
    groupvolume = _volume;
    sound_adjust_volume(sid, volume * groupvolume, 0);
}

float PSPChannel::getFinalVolume()
{
    return volume * groupvolume;
}

void PSPChannel::update()
{
    if (inuse)
    {
        if (!sound_is_playing(sid))
            stop();
    }
}

PSPBRIDGE(Channel,setVolume,(float volume),(volume))
FMOD_RESULT PSPChannel::setVolume(const float _volume, const bool setstate)
{
    if (setstate)
        volume = _volume;
    sound_adjust_volume(sid, volume * groupvolume, 0);
    return FMOD_OK;
}

PSPBRIDGE(Channel,getPosition,(unsigned int *position, FMOD_TIMEUNIT postype),(position,postype))
FMOD_RESULT PSPChannel::getPosition(unsigned int *position, FMOD_TIMEUNIT postype)
{
    assert(postype == FMOD_TIMEUNIT_MS);
    float secs = sound_playback_pos(sid);
    *position = (unsigned int) (secs * 1000.0f);
    return FMOD_OK;
}

PSPBRIDGE(Channel,getVolume,(float *volume),(volume))
FMOD_RESULT PSPChannel::getVolume(float *_volume)
{
    *_volume = volume;
    return FMOD_OK;
}

PSPBRIDGE(Channel,isPlaying,(bool *isplaying),(isplaying))
FMOD_RESULT PSPChannel::isPlaying(bool *isplaying)
{
    *isplaying = sound_is_playing(sid);
    return FMOD_OK;
}

PSPBRIDGE(Channel,setChannelGroup,(ChannelGroup *channelgroup),(channelgroup))
FMOD_RESULT PSPChannel::setChannelGroup(ChannelGroup *_channelgroup)
{
    PSPChannelGroup *channelgroup = ((PSPChannelGroup *) _channelgroup);
    assert(channelgroup);
    if (!channelgroup->attachChannel(this))
        return FMOD_ERR_INTERNAL;
    if ((group != NULL) && (group != channelgroup))
        group->detachChannel(this);
    group = channelgroup;
    return FMOD_OK;
}

PSPBRIDGE(Channel,setFrequency,(float frequency),(frequency))
FMOD_RESULT PSPChannel::setFrequency(const float _frequency)
{
    frequency = _frequency;
    if (sound_is_playing(sid))
        STUBBED("Can't change the pitch of a sound while playing");
    return FMOD_OK;
}

PSPBRIDGE(Channel,setPaused,(bool paused),(paused))
FMOD_RESULT PSPChannel::setPaused(const bool _paused, const bool setstate)
{
    if (_paused)
        sound_pause(sid);
    else
        sound_resume(sid);

    if (setstate)
        paused = _paused;

    return FMOD_OK;
}

PSPBRIDGE(Channel,setPriority,(int priority),(priority))
FMOD_RESULT PSPChannel::setPriority(int _priority)
{
    priority = _priority;
    return FMOD_OK;
}


// FMOD::ChannelGroup implementation...

PSPChannelGroup::PSPChannelGroup(const char *_name)
    : name(NULL)
    , paused(false)
    , channel_count(0)
    , channels(NULL)
    , volume(1.0f)
{
    if (_name)
    {
        char *buf = new char[strlen(_name) + 1];
        strcpy(buf, _name);
        name = buf;
    }
}

PSPChannelGroup::~PSPChannelGroup()
{
    delete[] name;
}

bool PSPChannelGroup::attachChannel(PSPChannel *channel)
{
    channel->setGroupVolume(volume);

    for (int i = 0; i < channel_count; i++)
    {
        if (channels[i] == channel)
            return true;
    }

    void *ptr = realloc(channels, sizeof (PSPChannel *) * (channel_count + 1));
    if (ptr == NULL)
        return false;

    channels = (PSPChannel **) ptr;
    channels[channel_count++] = channel;
    return true;
}

void PSPChannelGroup::detachChannel(PSPChannel *channel)
{
    for (int i = 0; i < channel_count; i++)
    {
        if (channels[i] == channel)
        {
            if (i < (channel_count-1))
                memmove(&channels[i], &channels[i+1], sizeof (PSPChannel *) * ((channel_count - i) - 1));
            channel_count--;
            return;
        }
    }

    assert(false && "Detached a channel that isn't part of the group!");
}


PSPBRIDGE(ChannelGroup,addDSP,(DSP *dsp, DSPConnection **connection),(dsp,connection))
FMOD_RESULT PSPChannelGroup::addDSP(DSP *dsp, DSPConnection **connection)
{
    STUBBED("Not yet implemented in PSP driver");
    return FMOD_ERR_INTERNAL;
}

PSPBRIDGE(ChannelGroup,getPaused,(bool *paused),(paused))
FMOD_RESULT PSPChannelGroup::getPaused(bool *_paused)
{
    *_paused = paused;
    return FMOD_OK;
}

PSPBRIDGE(ChannelGroup,getVolume,(float *volume),(volume))
FMOD_RESULT PSPChannelGroup::getVolume(float *_volume)
{
    *_volume = volume;
    return FMOD_OK;
}

PSPBRIDGE(ChannelGroup,setPaused,(bool paused),(paused))
FMOD_RESULT PSPChannelGroup::setPaused(const bool _paused)
{
    for (int i = 0; i < channel_count; i++)
        channels[i]->setPaused(_paused, false);
    paused = _paused;
    return FMOD_OK;
}

PSPBRIDGE(ChannelGroup,setVolume,(float volume),(volume))
FMOD_RESULT PSPChannelGroup::setVolume(const float _volume)
{
    volume = _volume;
    for (int i = 0; i < channel_count; i++)
        channels[i]->setGroupVolume(_volume);
    return FMOD_OK;
}

PSPBRIDGE(ChannelGroup,stop,(),())
FMOD_RESULT PSPChannelGroup::stop()
{
    for (int i = 0; i < channel_count; i++)
        channels[i]->stop();
    return FMOD_OK;
}


// FMOD::DSP implementation...

FMOD_RESULT DSP::getActive(bool *active)
{
    STUBBED("Not implemented");
    *active = false;
    return FMOD_ERR_INTERNAL;
}

FMOD_RESULT DSP::remove()
{
    STUBBED("Not implemented");
    return FMOD_ERR_INTERNAL;
}

FMOD_RESULT DSP::setParameter(int index, float value)
{
    STUBBED("Not implemented");
    return FMOD_ERR_INTERNAL;
}


// FMOD::Sound implementation ...

class PSPSound
{
public:
    PSPSound(void *_buffer, unsigned int _buflen,
             SysFile *_file, uint32_t _fileofs, uint32_t _filesize,
             SoundFormat _format, bool _looping);
    bool isLooping() const               { return looping; }
    bool isBuffer() const                { return buffer != NULL; }
    const void *getBuffer() const        { return buffer; }
    unsigned int getBufferLength() const { return buflen; }
    SysFile *getFile() const             { return file; }
    uint32_t getFileOffset() const       { return fileofs; }
    uint32_t getFileSize() const         { return filesize; }
    SoundFormat getFormat() const        { return format; }
    void clearFile()                     { file = NULL; }
    FMOD_RESULT release();
    void reference() { refcount++; }

private:
    void * const buffer;        // For files buffered in memory
    const unsigned int buflen;  // For files buffered in memory
    SysFile *file;              // For streamed files
    const uint32_t fileofs;     // For streamed files
    const uint32_t filesize;    // For streamed files
    const SoundFormat format;
    const bool looping;
    int refcount;
};

PSPSound::PSPSound(void *_buffer, unsigned int _buflen,
                   SysFile *_file, uint32_t _fileofs, uint32_t _filesize,
                   SoundFormat _format, bool _looping)
    : buffer(_buffer)
    , buflen(_buflen)
    , file(_file)
    , fileofs(_fileofs)
    , filesize(_filesize)
    , format(_format)
    , looping(_looping)
    , refcount(1)
{
}

PSPBRIDGE(Sound,release,(),())
FMOD_RESULT PSPSound::release()
{
    refcount--;
    if (refcount <= 0)
    {
        mem_free(buffer);
        sys_file_close(file);
        delete this;
    }
    return FMOD_OK;
}


void PSPChannel::setSound(PSPSound *_sound)
{
    if (sound)
        sound->release();

    sound = _sound;

    if (sound)
        sound->reference();
}


PSPBRIDGE(Channel,stop,(),())
FMOD_RESULT PSPChannel::stop()
{
    sound_cut(sid);
    if (sound)
    {
        sound->release();
        sound = NULL;
    }
    paused = false;
    inuse = false;
    return FMOD_OK;
}



// FMOD::System implementation ...

class PSPSystem
{
public:
    PSPSystem();
    ~PSPSystem();
    FMOD_RESULT init(const int maxchannels, const FMOD_INITFLAGS flags, const void *extradriverdata);
    FMOD_RESULT update();
    FMOD_RESULT release();
    FMOD_RESULT getVersion(unsigned int *version);
    FMOD_RESULT setSpeakerMode(const FMOD_SPEAKERMODE speakermode);
    FMOD_RESULT setFileSystem(FMOD_FILE_OPENCALLBACK useropen, FMOD_FILE_CLOSECALLBACK userclose, FMOD_FILE_READCALLBACK userread, FMOD_FILE_SEEKCALLBACK userseek, const int blockalign);
    FMOD_RESULT setDSPBufferSize(const unsigned int bufferlength, const int numbuffers);
    FMOD_RESULT createChannelGroup(const char *name, ChannelGroup **channelgroup);
    FMOD_RESULT createDSPByType(const FMOD_DSP_TYPE type, DSP **dsp);
    FMOD_RESULT createSound(const char *name_or_data, const FMOD_MODE mode, const FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound);
    FMOD_RESULT createStream(const char *name_or_data, const FMOD_MODE mode, const FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound);
    FMOD_RESULT getDriverCaps(const int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency, FMOD_SPEAKERMODE *controlpanelspeakermode);
    FMOD_RESULT getMasterChannelGroup(ChannelGroup **channelgroup);
    FMOD_RESULT playSound(FMOD_CHANNELINDEX channelid, Sound *sound, bool paused, Channel **channel);

private:
    PSPChannelGroup *master_channel_group;
    int num_channels;
    PSPChannel *channels;
};


PSPSystem::PSPSystem()
    : master_channel_group(NULL)
    , num_channels(0)
    , channels(NULL)
{
}

PSPSystem::~PSPSystem()
{
    delete master_channel_group;
    delete[] channels;
}


FMOD_RESULT System_Create(FMOD_SYSTEM **system)
{
    *system = (FMOD_SYSTEM *) new PSPSystem;
    return FMOD_OK;
}

PSPBRIDGE(System,createChannelGroup,(const char *name, ChannelGroup **channelgroup),(name,channelgroup))
FMOD_RESULT PSPSystem::createChannelGroup(const char *name, ChannelGroup **channelgroup)
{
    *channelgroup = (ChannelGroup *) new PSPChannelGroup(name);
    return FMOD_OK;
}

PSPBRIDGE(System,createDSPByType,(FMOD_DSP_TYPE type, DSP **dsp),(type,dsp))
FMOD_RESULT PSPSystem::createDSPByType(const FMOD_DSP_TYPE type, DSP **dsp)
{
    *dsp = NULL;
    STUBBED("Not implemented");
    return FMOD_ERR_INTERNAL;
}

/* Open the named sound file, preferring MP3 to Ogg. */
static SysFile *psp_open_sound(const char *_path, uint32_t &fileofs,
                               uint32_t &filesize, SoundFormat &format)
{
    const unsigned int bufsize = strlen(_path) + 5;
    char *path = (char *)alloca(bufsize);
    strcpy(path, _path);
    char *ptr = strrchr(path, '.');
    if (ptr) *ptr = '\0';
    ptr = path + strlen(path);

    SysFile *f;
    memcpy(ptr, ".mp3", 5);
    f = resource_open_as_file(path, &fileofs, &filesize);
    if (f) {
        format = SOUND_FORMAT_MP3;
        return f;
    }

    memcpy(ptr, ".ogg", 5);
    f = resource_open_as_file(path, &fileofs, &filesize);
    if (f) {
        format = SOUND_FORMAT_OGG;
        return f;
    }

    return NULL;
}

PSPBRIDGE(System,createSound,(const char *name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound),(name_or_data,mode,exinfo,sound))
FMOD_RESULT PSPSystem::createSound(const char *name_or_data, const FMOD_MODE mode, const FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound)
{
    assert(!exinfo);

    uint32_t fileofs = 0, filesize = 0;
    SoundFormat format;
    SysFile *file = psp_open_sound(name_or_data, fileofs, filesize, format);
    if (file == NULL)
        return FMOD_ERR_INTERNAL;

    void *buffer = NULL;
    if (!(mode & FMOD_CREATESTREAM)) {
        // Allocate sound buffers from the top of memory, so they don't
        // contribute to fragmentation of the area used by malloc().
        buffer = mem_alloc(filesize, 64, MEM_ALLOC_TOP);
        if (buffer == NULL) {
            sys_file_close(file);
            return FMOD_ERR_INTERNAL;
        }
        unsigned int nread = sys_file_read(file, buffer, filesize);
        sys_file_close(file);
        file = NULL;
        if (nread != filesize) {
            mem_free(buffer);
            return FMOD_ERR_INTERNAL;
        }
    }

    *sound = (Sound *) new PSPSound(buffer, filesize, file, fileofs, filesize,
                                    format, (((mode & FMOD_LOOP_OFF) == 0) && (mode & FMOD_LOOP_NORMAL)));
    return FMOD_OK;
}

PSPBRIDGE(System,createStream,(const char *name_or_data, FMOD_MODE mode, FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound),(name_or_data,mode,exinfo,sound))
FMOD_RESULT PSPSystem::createStream(const char *name_or_data, const FMOD_MODE mode, const FMOD_CREATESOUNDEXINFO *exinfo, Sound **sound)
{
    return createSound(name_or_data, mode | FMOD_CREATESTREAM, exinfo, sound);
}

PSPBRIDGE(System,getDriverCaps,(int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency, FMOD_SPEAKERMODE *controlpanelspeakermode),(id,caps,minfrequency,maxfrequency,controlpanelspeakermode))
FMOD_RESULT PSPSystem::getDriverCaps(const int id, FMOD_CAPS *caps, int *minfrequency, int *maxfrequency, FMOD_SPEAKERMODE *controlpanelspeakermode)
{
    assert(!id);
    assert(!minfrequency);
    assert(!maxfrequency);
    *controlpanelspeakermode = FMOD_SPEAKERMODE_STEREO;  // not strictly true, but works for aquaria's usage.
    *caps = 0;   // aquaria only checks FMOD_CAPS_HARDWARE_EMULATED.
    return FMOD_OK;
}

PSPBRIDGE(System,getMasterChannelGroup,(ChannelGroup **channelgroup),(channelgroup))
FMOD_RESULT PSPSystem::getMasterChannelGroup(ChannelGroup **channelgroup)
{
    *channelgroup = (ChannelGroup *) master_channel_group;
    return FMOD_OK;
}

PSPBRIDGE(System,getVersion,(unsigned int *version),(version))
FMOD_RESULT PSPSystem::getVersion(unsigned int *version)
{
    *version = FMOD_VERSION;
    return FMOD_OK;
}

PSPBRIDGE(System,init,(int maxchannels, FMOD_INITFLAGS flags, void *extradriverdata),(maxchannels,flags,extradriverdata))
FMOD_RESULT PSPSystem::init(const int maxchannels, const FMOD_INITFLAGS flags, const void *extradriverdata)
{
    master_channel_group = new PSPChannelGroup("master");

    num_channels = maxchannels;
    channels = new PSPChannel[maxchannels];
    for (int i = 0; i < num_channels; i++)
    {
        channels[i].setSourceName(i+1);
        channels[i].setChannelGroup((ChannelGroup *) master_channel_group);
    }
    return FMOD_OK;
}

PSPBRIDGE(System,playSound,(FMOD_CHANNELINDEX channelid, Sound *sound, bool paused, Channel **channel),(channelid,sound,paused,channel))
FMOD_RESULT PSPSystem::playSound(FMOD_CHANNELINDEX channelid, Sound *_sound, bool paused, Channel **channel)
{
    *channel = NULL;

    if (channelid == FMOD_CHANNEL_FREE)
    {
        for (int i = 0; i < num_channels; i++)
        {
            if (!channels[i].isInUse())
            {
                channelid = (FMOD_CHANNELINDEX) i;
                break;
            }
        }
    }

    if ((channelid < 0) || (channelid >= num_channels))
        return FMOD_ERR_INTERNAL;

    PSPSound *sound = (PSPSound *) _sound;
    const unsigned int sid = channels[channelid].getSourceName();
    const float volume = channels[channelid].getFinalVolume();
    sound_cut(sid);
    if (sound->isBuffer())
    {
        sound_play_buffer(sid, sound->getFormat(), sound->getBuffer(),
                          sound->getBufferLength(), volume, 0,
                          sound->isLooping());
    }
    else
    {
        sound_play_file(sid, sound->getFormat(), sound->getFile(),
                        sound->getFileOffset(), sound->getFileSize(),
                        volume, 0, sound->isLooping());
	sound->clearFile();  // Will be closed by the sound subsystem.
    }

    channels[channelid].reacquire();
    channels[channelid].setPaused(paused);
    channels[channelid].setSound(sound);
    *channel = (Channel *) &channels[channelid];
    return FMOD_OK;
}


PSPBRIDGE(System,release,(),())
FMOD_RESULT PSPSystem::release()
{
    for (int i = 0; i < num_channels; i++)
    {
        sound_cut(i+1);
    }
    delete this;
    return FMOD_OK;
}

PSPBRIDGE(System,setDSPBufferSize,(unsigned int bufferlength, int numbuffers),(bufferlength,numbuffers))
FMOD_RESULT PSPSystem::setDSPBufferSize(const unsigned int bufferlength, const int numbuffers)
{
    // aquaria only uses this for FMOD_CAPS_HARDWARE_EMULATED, so I skipped it.
    return FMOD_ERR_INTERNAL;
}

PSPBRIDGE(System,setFileSystem,(FMOD_FILE_OPENCALLBACK useropen, FMOD_FILE_CLOSECALLBACK userclose, FMOD_FILE_READCALLBACK userread, FMOD_FILE_SEEKCALLBACK userseek, int blockalign),(useropen,userclose,userread,userseek,blockalign))
FMOD_RESULT PSPSystem::setFileSystem(FMOD_FILE_OPENCALLBACK useropen, FMOD_FILE_CLOSECALLBACK userclose, FMOD_FILE_READCALLBACK userread, FMOD_FILE_SEEKCALLBACK userseek, const int blockalign)
{
    // Aquaria sets these, but they don't do anything fancy, so we ignore them for now.
    return FMOD_OK;
}

PSPBRIDGE(System,setSpeakerMode,(FMOD_SPEAKERMODE speakermode),(speakermode))
FMOD_RESULT PSPSystem::setSpeakerMode(const FMOD_SPEAKERMODE speakermode)
{
    return FMOD_OK;  // we ignore this for Aquaria.
}

PSPBRIDGE(System,update,(),())
FMOD_RESULT PSPSystem::update()
{
    for (int i = 0; i < num_channels; i++)
        channels[i].update();
    return FMOD_OK;
}


// misc FMOD bits...

FMOD_RESULT Memory_GetStats(int *currentalloced, int *maxalloced, FMOD_BOOL blocking)
{
    // not ever used by Aquaria.
    *currentalloced = *maxalloced = 42;
    return FMOD_ERR_INTERNAL;
}

}  // namespace FMOD

// end of FmodPSPBridge.cpp ...

