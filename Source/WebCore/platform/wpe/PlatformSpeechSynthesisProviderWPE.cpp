
#include "config.h"
#if ENABLE(SPEECH_SYNTHESIS)
#ifndef FLITE_C_INCLUDED
#define FLITE_C_INCLUDED
extern "C" {
#include <flite_voice_list.c>
}
#endif
#include <NotImplemented.h>
#include <PlatformSpeechSynthesisUtterance.h>
#include <PlatformSpeechSynthesisVoice.h>
#include <PlatformSpeechSynthesizer.h>
#include <wtf/text/CString.h>
#include <unistd.h>

#include "PlatformSpeechSynthesisProviderWPE.h"

/*********** APLAY *********/
#include <stdio.h>
#include <malloc.h>
#include <unistd.h>
#include <stdlib.h>
#include <locale.h>
#include <alsa/asoundlib.h>

#include <gettext.h>
#include <formats.h>
#include <version.h>

#ifdef SND_CHMAP_API_VERSION
#define CONFIG_SUPPORT_CHMAP    1
#endif

#ifndef LLONG_MAX
#define LLONG_MAX    9223372036854775807LL
#endif

#ifndef le16toh
#include <asm/byteorder.h>
#define le16toh(x) __le16_to_cpu(x)
#define be16toh(x) __be16_to_cpu(x)
#define le32toh(x) __le32_to_cpu(x)
#define be32toh(x) __be32_to_cpu(x)
#endif

#define DEFAULT_FORMAT          SND_PCM_FORMAT_U8
#define DEFAULT_SPEED           8000

#define FORMAT_WAVE             2
/* global data */
static snd_pcm_sframes_t (*writei_func)(snd_pcm_t *handle, const void *buffer, snd_pcm_uframes_t size);
static snd_pcm_t *handle;
static struct {
        snd_pcm_format_t format;
        unsigned int channels;
        unsigned int rate;
} hwparams, rhwparams;
static int timelimit = 0;
static int quiet_mode = 0;
static int open_mode = 0;
static snd_pcm_stream_t stream = SND_PCM_STREAM_PLAYBACK;
static int mmap_flag = 0;
static int nonblock = 0;
static u_char *audiobuf = NULL;
static snd_pcm_uframes_t chunk_size = 0;
static unsigned period_time = 0;
static unsigned buffer_time = 0;
static snd_pcm_uframes_t period_frames = 0;
static snd_pcm_uframes_t buffer_frames = 0;
static int avail_min = -1;
static int start_delay = 0;
static int stop_delay = 0;
static int monotonic = 0;
//static int interactive = 0;
static int can_pause = 0;
static size_t significant_bits_per_sample, bits_per_sample, bits_per_frame;
static size_t chunk_bytes;
static int test_nowait = 0;
volatile static int recycle_capture_file = 0;
static int fd = -1;
static off64_t pbrec_count = LLONG_MAX, fdcount;

#ifdef CONFIG_SUPPORT_CHMAP
static unsigned int *hw_map = NULL; /* chmap to follow */
#endif


/**********************************************************************/

namespace WebCore {

/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

PlatformSpeechSynthesisProviderWPE::PlatformSpeechSynthesisProviderWPE(PlatformSpeechSynthesizer* client)
    : m_fliteInited(false)
    , m_speechDuration(0.0)
    , m_platformSpeechSynthesizer(client)
{
    flite_init(); 
    m_fliteInited = true;
    printf ("This is line %d of file %s (function %s) FLITE INITED \n",__LINE__, __FILE__, __func__);
}

/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

PlatformSpeechSynthesisProviderWPE::~PlatformSpeechSynthesisProviderWPE()
{
    delete_val(flite_voice_list); flite_voice_list=0;

}
/***********************************************************************
* @brief : Init the  Voice list
* @Function Name : void initializeVoiceList(Vector<RefPtr<PlatformSpeechSynthesisVoice>>&);
*
* @param  : Vector<RefPtr<PlatformSpeechSynthesisVoice>>& ,
*           Reference to voice list.  
* @return : Void 
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::initializeVoiceList(Vector<RefPtr<PlatformSpeechSynthesisVoice>>& voiceList)
{
    cst_voice *voice;
    const cst_val *v;
    WTF::String voiceId = "default";
    WTF::String langSupport = "en-US";
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    
    flite_voice_list = flite_set_voice_list();
    for (v=flite_voice_list; v; v=val_cdr(v))
    {
        voice = val_voice(val_car(v));
        /*Convert the  voice to human readable form */
           
        String voiceName = voice->name;
        if(voiceName.contains("kal",true))
        {
            printf("Kal sound is heard \n");
            langSupport = "en-US" ;
            voiceName = "US English Male";
        } 
        else if(voiceName.contains("rms",true))
        {
            printf("rms sound is heard \n");
            langSupport = "en-US" ;
            voiceName = "US English Male";
        } 

        else if(voiceName.contains("awb",true))
        {
            printf("awb sound is heard \n");
            langSupport = "en-Scott" ;
            voiceName = "Scottish English Male";
        } 
        else if(voiceName.contains("slt",true))
        {
            printf("slt sound is heard \n");
            langSupport = "en-US" ;
            voiceName = "US English Female";
        } 
        else
        {
            printf("Support for this language has to be add \n");
        }
        voiceList.append(PlatformSpeechSynthesisVoice::create(String(voiceId), voiceName,langSupport, true, true));
    }

}

void PlatformSpeechSynthesisProviderWPE::pause()
{ 
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    do_pause();
}

/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::resume()
{
    notImplemented();
}

/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::speak(PassRefPtr<PlatformSpeechSynthesisUtterance> utterance)
{
   // m_utterance = utterance;
    cst_voice *v;
    if(! m_fliteInited)
        printf("m_fliteInited is not set \n");
    if(!utterance)
        printf(" utterance is not set \n");
   #if 0 
    if (! m_fliteInited || !utterance) {
        fireSpeechEvent(SpeechError);
        return;
    }
    #endif
    v = register_cmu_us_kal(NULL); //TODO: 
                                   // this has to be set based on user settings
    m_speechDuration =  flite_text_to_speech((m_utterance->text()).utf8().data(),v,"/tmp/speechsynth.wav");
    printf ("This is line %d of file %s (function %s) speak for  duration= %f\n",__LINE__, __FILE__, __func__,m_speechDuration);
    speechMain();
    fireSpeechEvent(SpeechStart);
    sleep(int(m_speechDuration));
    fireSpeechEvent(SpeechEnd);
}

/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::cancel()
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    if (!m_fliteInited || !m_utterance)
        return;
   //TODO 
       
    fireSpeechEvent(SpeechCancel);
    m_utterance = nullptr;

}

/***********************************************************************
* @brief :  To fire  platform events to the generic module 
* @Function Name : void fireSpeechEvent(SpeechEvent speechEvent)
* @param : SpeechEvent - Event marker
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::fireSpeechEvent(SpeechEvent speechEvent)
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
    switch (speechEvent) {
    case SpeechStart:
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        m_platformSpeechSynthesizer->client()->didStartSpeaking(m_utterance);
        break;
    case SpeechPause:
        m_platformSpeechSynthesizer->client()->didPauseSpeaking(m_utterance);
        break;
    case SpeechResume:
        m_platformSpeechSynthesizer->client()->didResumeSpeaking(m_utterance);
        break;
    case SpeechError:
        m_fliteInited = false;
    case SpeechCancel:
        m_platformSpeechSynthesizer->client()->speakingErrorOccurred(m_utterance);
        break;
    case SpeechEnd:
        printf ("This is line %d of file %s (function %s) \n",__LINE__, __FILE__, __func__);
        m_platformSpeechSynthesizer->client()->didFinishSpeaking(m_utterance);
        break;
    default:
        ASSERT_NOT_REACHED();
    };
}

/* Aplay playback */
/*
 *      Subroutine to clean up before exit.
 */
/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::prg_exit(int code)
{
//TODO
}
/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/


int PlatformSpeechSynthesisProviderWPE::speechMain()
{
        char *pcm_name = "plughw:1";
        int err;
        /*********************/
        stream = SND_PCM_STREAM_PLAYBACK;
        /*******************/
        chunk_size = -1;
        rhwparams.format = DEFAULT_FORMAT;
        rhwparams.rate = DEFAULT_SPEED;
        rhwparams.channels = 1;
        err = snd_pcm_open(&handle, pcm_name, stream, open_mode);
        if (err < 0) {
                //error(_("audio open error: %s"), snd_strerror(err));
                return 1;
        }
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        chunk_size = 1024;
        hwparams = rhwparams;
        audiobuf = (u_char *)malloc(1024);
        if (audiobuf == NULL) {
         //error(_("not enough memory"));
                return 1;
        }
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        writei_func = snd_pcm_writei;
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        playback("/tmp/speechsynth.wav");
        snd_pcm_close(handle);
        handle = NULL;
        printf ("This is line %d of file %s (function %s) free fn\n",__LINE__, __FILE__, __func__);
        free(audiobuf);
        snd_config_update_free_global();
        printf ("This is line %d of file %s (function %s) free fn ...\n",__LINE__, __FILE__, __func__);
        prg_exit(EXIT_SUCCESS);
        /* avoid warning */
        return EXIT_SUCCESS;
}

/*
 * Safe read (for pipes)
 */
/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

ssize_t PlatformSpeechSynthesisProviderWPE::safe_read(int fd, void *buf, size_t count)
{
    ssize_t result = 0, res;
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);

        while (count > 0 ) {
                printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
                if ((res = read(fd, buf, count)) == 0)
                        break;
                if (res < 0)
                        return result > 0 ? result : res;
                count -= res;
                result += res;
                buf = (char *)buf + res;
        }
        return result;
}
/*
 * helper for test_wavefile
 */
/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

size_t PlatformSpeechSynthesisProviderWPE::test_wavefile_read(int fd, u_char *buffer, size_t *size, size_t reqsize)
{
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        if (*size >= reqsize)
                return *size;
        if ((size_t)safe_read(fd, buffer + *size, reqsize - *size) != reqsize - *size) {
                 printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
                //error(_("read error (called from line %i)"), line);
                prg_exit(EXIT_FAILURE);
        }
        return *size = reqsize;
}
#define check_wavefile_space(buffer, len, blimit) \
        if (len > blimit) { \
                blimit = len; \
                if ((buffer = realloc(buffer, blimit)) == NULL) { \
                        /*error(_("not enough memory"));           */ \
                        prg_exit(EXIT_FAILURE);  \
                } \
        }

/*
 * test, if it's a .WAV file, > 0 if ok (and set the speed, stereo etc.)
 *                            == 0 if not
 * Value returned is bytes to be discarded.
 */
/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/
ssize_t PlatformSpeechSynthesisProviderWPE::test_wavefile(int fd, u_char *_buffer, size_t size)
{
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        WaveHeader *h = (WaveHeader *)_buffer;
        void *buffer = NULL;
        size_t blimit = 0;
        WaveFmtBody *f;
        WaveChunkHeader *c;
        u_int type, len;
        unsigned short format, channels;
        int big_endian, native_format;

        if (size < sizeof(WaveHeader))
                return -1;
        if (h->magic == WAV_RIFF)
         {
                printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
                big_endian = 0;
         }
        if (size > sizeof(WaveHeader)) {
                printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
                check_wavefile_space(buffer, size - sizeof(WaveHeader), blimit);
                memcpy(buffer, _buffer + sizeof(WaveHeader), size - sizeof(WaveHeader));
        }
        size -= sizeof(WaveHeader);
        while (1) {
                printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
                check_wavefile_space(buffer, sizeof(WaveChunkHeader), blimit);
                test_wavefile_read(fd, (u_char *)buffer, &size, sizeof(WaveChunkHeader));
                c = (WaveChunkHeader*)buffer;
                type = c->type;
                len = TO_CPU_INT(c->length, big_endian);
                len += len % 2;
                if (size > sizeof(WaveChunkHeader))
                        memmove(buffer, buffer + sizeof(WaveChunkHeader), size - sizeof(WaveChunkHeader));
                size -= sizeof(WaveChunkHeader);
                if (type == WAV_FMT)
                        break;
                check_wavefile_space(buffer, len, blimit);
                test_wavefile_read(fd, (u_char *)buffer, &size, len);
                if (size > len)
                        memmove(buffer, buffer + len, size - len);
                size -= len;
        }
        check_wavefile_space(buffer, len, blimit);
        test_wavefile_read(fd, (u_char *)buffer, &size, len);
        f = (WaveFmtBody*) buffer;
        format = TO_CPU_SHORT(f->format, big_endian);
        native_format = SND_PCM_FORMAT_S16_LE; //format 
        if (hwparams.format != DEFAULT_FORMAT &&
                    hwparams.format != native_format)
        fprintf(stderr, _("Warning: format is changed to %s\n"),
                                snd_pcm_format_name((snd_pcm_format_t)native_format));
        hwparams.format = (snd_pcm_format_t)native_format;
        if (size > len)
                memmove(buffer, buffer + len, size - len);
        size -= len;

        while (1) {
                printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
                u_int type, len;
                check_wavefile_space(buffer, sizeof(WaveChunkHeader), blimit);
                test_wavefile_read(fd, (u_char *)buffer, &size, sizeof(WaveChunkHeader));
                c = (WaveChunkHeader*)buffer;
                type = c->type;
                len = TO_CPU_INT(c->length, big_endian);
                if (size > sizeof(WaveChunkHeader))
                        memmove(buffer, buffer + sizeof(WaveChunkHeader), size - sizeof(WaveChunkHeader));
                size -= sizeof(WaveChunkHeader);
                if (type == WAV_DATA) {
                        if (len < pbrec_count && len < 0x7ffffffe)
                                pbrec_count = len;
                        if (size > 0)
                                memcpy(_buffer, buffer, size);
                        free(buffer);
                        return size;
                }
                len += len % 2;
                check_wavefile_space(buffer, len, blimit);
                test_wavefile_read(fd, (u_char *)buffer, &size, len);
                if (size > len)
                        memmove(buffer, buffer + len, size - len);
                size -= len;
        }

 /* shouldn't be reached */
        return -1;
}
/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::set_params(void)
{
        snd_pcm_hw_params_t *params;
        snd_pcm_sw_params_t *swparams;
        snd_pcm_uframes_t buffer_size;
        int err;
        size_t n;
        unsigned int rate;
        snd_pcm_uframes_t start_threshold, stop_threshold;
        snd_pcm_hw_params_alloca(&params);
        snd_pcm_sw_params_alloca(&swparams);
        err = snd_pcm_hw_params_any(handle, params);
        if (err < 0) {
                //error(_("Broken configuration for this PCM: no configurations available"));
                prg_exit(EXIT_FAILURE);
        }
                err = snd_pcm_hw_params_set_access(handle, params,
                                                   SND_PCM_ACCESS_RW_INTERLEAVED);
        if (err < 0) {
                //error(_("Access type not available"));
                prg_exit(EXIT_FAILURE);
        }
        err = snd_pcm_hw_params_set_format(handle, params, hwparams.format);
        if (err < 0) {
                //error(_("Sample format non available"));
                //show_available_sample_formats(params);
                prg_exit(EXIT_FAILURE);
        }
        err = snd_pcm_hw_params_set_channels(handle, params, hwparams.channels);
        if (err < 0) {
               // error(_("Channels count non available"));
                prg_exit(EXIT_FAILURE);
        } 
         rate = hwparams.rate;
        err = snd_pcm_hw_params_set_rate_near(handle, params, &hwparams.rate, 0);
        assert(err >= 0);
        if ((float)rate * 1.05 < hwparams.rate || (float)rate * 0.95 > hwparams.rate) {
                if (!quiet_mode) {
                        char plugex[64];
                        const char *pcmname = snd_pcm_name(handle);
                        fprintf(stderr, _("Warning: rate is not accurate (requested = %iHz, got = %iHz)\n"), rate, hwparams.rate);
                        if (! pcmname || strchr(snd_pcm_name(handle), ':'))
                                *plugex = 0;
                        else
                                snprintf(plugex, sizeof(plugex), "(-Dplug:%s)",
                                         snd_pcm_name(handle));
                        fprintf(stderr, _("         please, try the plug plugin %s\n"),
                                plugex);
                }
        }
        rate = hwparams.rate;
        if (buffer_time == 0 && buffer_frames == 0) {
                err = snd_pcm_hw_params_get_buffer_time_max(params,
                                                            &buffer_time, 0);
                assert(err >= 0);
                if (buffer_time > 500000)
                        buffer_time = 500000;
        }
        if (period_time == 0 && period_frames == 0) {
                if (buffer_time > 0)
                        period_time = buffer_time / 4;
                else
                        period_frames = buffer_frames / 4;
        }
        if (period_time > 0)
                err = snd_pcm_hw_params_set_period_time_near(handle, params,
                                                             &period_time, 0);
        else
                err = snd_pcm_hw_params_set_period_size_near(handle, params,
                                                             &period_frames, 0);
        assert(err >= 0);
        if (buffer_time > 0) {
                err = snd_pcm_hw_params_set_buffer_time_near(handle, params,
                                                             &buffer_time, 0);
        } else {
                err = snd_pcm_hw_params_set_buffer_size_near(handle, params,
                                                             &buffer_frames);
        }
        assert(err >= 0);
        monotonic = snd_pcm_hw_params_is_monotonic(params);
        can_pause = snd_pcm_hw_params_can_pause(params);
        err = snd_pcm_hw_params(handle, params);
        if (err < 0) {
                //error(_("Unable to install hw params:"));
//              snd_pcm_hw_params_dump(params, log);
                prg_exit(EXIT_FAILURE);
        }
        snd_pcm_hw_params_get_period_size(params, &chunk_size, 0);
        snd_pcm_hw_params_get_buffer_size(params, &buffer_size);
        if (chunk_size == buffer_size) {
                //error(_("Can't use period equal to buffer size (%lu == %lu)"),
                     // chunk_size, buffer_size);
                prg_exit(EXIT_FAILURE);
        }
        snd_pcm_sw_params_current(handle, swparams);
        if (avail_min < 0)
                n = chunk_size;
        else
                n = (double) rate * avail_min / 1000000;
        err = snd_pcm_sw_params_set_avail_min(handle, swparams, n);

        /* round up to closest transfer boundary */
        n = buffer_size;
        if (start_delay <= 0) {
                start_threshold = n + (double) rate * start_delay / 1000000;
        } else
                start_threshold = (double) rate * start_delay / 1000000;
        if (start_threshold < 1)
                start_threshold = 1;
        if (start_threshold > n)
                start_threshold = n;
        err = snd_pcm_sw_params_set_start_threshold(handle, swparams, start_threshold);
        assert(err >= 0);
        if (stop_delay <= 0)
                stop_threshold = buffer_size + (double) rate * stop_delay / 1000000;
        else
                stop_threshold = (double) rate * stop_delay / 1000000;
        err = snd_pcm_sw_params_set_stop_threshold(handle, swparams, stop_threshold);
        assert(err >= 0);

        if (snd_pcm_sw_params(handle, swparams) < 0) {
                //error(_("unable to install sw params:"));
                prg_exit(EXIT_FAILURE);
        }
        bits_per_sample = snd_pcm_format_physical_width(hwparams.format);
        significant_bits_per_sample = snd_pcm_format_width(hwparams.format);
        bits_per_frame = bits_per_sample * hwparams.channels;
        chunk_bytes = chunk_size * bits_per_frame / 8;
        audiobuf = (u_char*)realloc(audiobuf, chunk_bytes);
        if (audiobuf == NULL) {
                //error(_("not enough memory"));
                prg_exit(EXIT_FAILURE);
        }

        /* show mmap buffer arragment */
        if (mmap_flag ) {
                const snd_pcm_channel_area_t *areas;
                snd_pcm_uframes_t offset, size = chunk_size;
                int i;
                err = snd_pcm_mmap_begin(handle, &areas, &offset, &size);
                if (err < 0) {
                        //error(_("snd_pcm_mmap_begin problem: %s"), snd_strerror(err));
                        prg_exit(EXIT_FAILURE);
                }
                for (i = 0; i < hwparams.channels; i++)
                        fprintf(stderr, "mmap_area[%i] = %p,%u,%u (%u)\n", i, areas[i].addr, areas[i].first, areas[i].step, snd_pcm_format_physical_width(hwparams.format));
                /* not required, but for sure */
                snd_pcm_mmap_commit(handle, offset, 0);
        }

        buffer_frames = buffer_size;    /* for position test */
}
#if 1
/***********************************************************************
* @brief 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/
void PlatformSpeechSynthesisProviderWPE::do_pause(void)
{
    printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        int err;
        unsigned char b;

        if (!can_pause) {
                printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
                fprintf(stderr, _("\rPAUSE command ignored (no hw support)\n"));
                return;
        }
        err = snd_pcm_pause(handle, 1);
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        if (err < 0) {
              //  error(_("pause push error: %s"), snd_strerror(err));
                return;
        }
        while (1) {
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
                while (read(fileno(stdin), &b, 1) != 1);
                if (b == ' ' || b == '\r') {
                        while (read(fileno(stdin), &b, 1) == 1);
                        err = snd_pcm_pause(handle, 0);
                        if (err < 0)
                               // error(_("pause release error: %s"), snd_strerror(err));
                        return;
                }
        }
}
#endif
#ifdef CONFIG_SUPPORT_CHMAP
/***********************************************************************
* @brief : u_char *remap_data(u_char *data, size_t count) 
* @Function Name :
*
* @param <Type> <use>
* @param <Type> <use>
* @return <Type> <use>
***********************************************************************/
u_char *PlatformSpeechSynthesisProviderWPE::remap_data(u_char *data, size_t count)
{
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        static u_char *tmp, *src, *dst;
        static size_t tmp_size;
        size_t sample_bytes = bits_per_sample / 8;
        size_t step = bits_per_frame / 8;
        size_t chunk_bytes;
        unsigned int ch, i;

        if (!hw_map)
                return data;

        chunk_bytes = count * bits_per_frame / 8;
        if (tmp_size < chunk_bytes) {
                free(tmp);
                tmp =(u_char *) malloc(chunk_bytes);
                if (!tmp) {
                      //  error(_("not enough memory"));
                        exit(1);
                }
                tmp_size = count;
        }

        src = data;
        dst = tmp;
        for (i = 0; i < count; i++) {
                for (ch = 0; ch < hwparams.channels; ch++) {
                        memcpy(dst, src + sample_bytes * hw_map[ch],
                               sample_bytes);
                        dst += sample_bytes;
 }
                src += step;
        }
        return tmp;
}
#endif
/***********************************************************************
* @brief : Write function 
* @Function Name : ssize_t pcm_write(u_char *data, size_t count)
*
* @param u_char *data
* @param size_t count
***********************************************************************/

ssize_t PlatformSpeechSynthesisProviderWPE::pcm_write(u_char *data, size_t count)
{
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        ssize_t r;
        ssize_t result = 0;

        if (count < chunk_size) {
                snd_pcm_format_set_silence(hwparams.format, data + count * bits_per_frame / 8, (chunk_size - count) * hwparams.channels);
                count = chunk_size;
        }
        data = remap_data(data, count);
        while (count > 0 ) {
                r = writei_func(handle, data, count);
                if (r == -EAGAIN || (r >= 0 && (size_t)r < count)) {
                        if (!test_nowait)
                                snd_pcm_wait(handle, 100);
                }
                if (r > 0) {
                        result += r;
                        count -= r;
                        data += r * bits_per_frame / 8;
                }
        }
        return result;
}

/***********************************************************************
* @brief : calculate the data count to read from/to dsp  
* @Function Name : off64_t calc_count(void)
***********************************************************************/
off64_t PlatformSpeechSynthesisProviderWPE::calc_count(void)
{
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        off64_t count;

        if (timelimit == 0) {
                count = pbrec_count;
        } else {
                count = snd_pcm_format_size(hwparams.format, hwparams.rate * hwparams.channels);
                count *= (off64_t)timelimit;
        }
        return count < pbrec_count ? count : pbrec_count;
}

/***********************************************************************
* @brief : playing raw data 
* @Function Name : void playback_go(int fd, size_t loaded, off64_t count, char *name)
*
* @param int fd -  device
* @param  size_t loaded 
* @param  off64_t count 
* @param  char *name 
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::playback_go(int fd, size_t loaded, off64_t count, char *name)
{
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
	int l, r;
	off64_t written = 0;
	off64_t c;
	set_params();

	while (loaded > chunk_bytes && written < count ) {
		if (pcm_write(audiobuf + written, chunk_size) <= 0)
			return;
		written += chunk_bytes;
		loaded -= chunk_bytes;
	}
	if (written > 0 && loaded > 0)
		memmove(audiobuf, audiobuf + written, loaded);

	l = loaded;
	while (written < count ) {
		do {
			c = count - written;
			if (c > chunk_bytes)
				c = chunk_bytes;
			c -= l;

			if (c == 0)
				break;
			r = safe_read(fd, audiobuf + l, c);
			if (r < 0) {
				perror(name);
				prg_exit(EXIT_FAILURE);
			}
			fdcount += r;
			if (r == 0)
				break;
			l += r;
		} while ((size_t)l < chunk_bytes);
		l = l * 8 / bits_per_frame;
		r = pcm_write(audiobuf, l);
		if (r != l)
			break;
		r = r * bits_per_frame / 8;
		written += r;
		l = 0;
	}
	snd_pcm_nonblock(handle, 0);
	snd_pcm_drain(handle);
	snd_pcm_nonblock(handle, nonblock);
}


/***********************************************************************
* @brief : let's play WAVE 
* @Function Name : void playback(char *name)
*
* @param char *name - name of wav file to be palyed back
***********************************************************************/

void PlatformSpeechSynthesisProviderWPE::playback(char *name)
{
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
	size_t dta;
	ssize_t dtawave;

	pbrec_count = LLONG_MAX;
	fdcount = 0;
        printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
	if ((fd = open(name, O_RDONLY, 0)) == -1) {
		perror(name);
		prg_exit(EXIT_FAILURE);
	}
        printf ("This is line %d of file %s (function %s) File opened is %s \n",__LINE__, __FILE__, __func__,name);
	/* read the file header */
	dta = sizeof(AuHeader);
	if ((size_t)safe_read(fd, audiobuf, dta) != dta) {
                printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
		//error(_("read error"));
		prg_exit(EXIT_FAILURE);
	}else{
                printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        }

	dta = sizeof(VocHeader);
	if ((size_t)safe_read(fd, audiobuf + sizeof(AuHeader),
		 dta - sizeof(AuHeader)) != dta - sizeof(AuHeader)) {
		//error(_("read error"));
		prg_exit(EXIT_FAILURE);;
	
	}else{
                printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
        }
	/* read bytes for WAVE-header */
	if ((dtawave = test_wavefile(fd, audiobuf, dta)) >= 0) {
                printf ("This is line %d of file %s (function %s)\n",__LINE__, __FILE__, __func__);
		pbrec_count = calc_count();
		playback_go(fd, dtawave, pbrec_count,name);
	}
	if (fd != 0)
		close(fd);
  }  
} // namespace WebCore

#endif //speech synthesis
