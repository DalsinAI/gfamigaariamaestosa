/* Aria Maestosa's MIDI player for the Amiga: the song as Aria writes it for
 * playback (allocAsMidiBytes), parsed by camd_smf.c and sent to a CAMD
 * cluster by camd_player.c's own process. Preview notes go straight out.
 * Recording (MIDI in) and audio export are not in this port.
 * GPL-2.0-or-later, like Aria Maestosa. Copyright (c) 2026 AmiCradle Team. */
#ifdef __amigaos__

#include "AriaCore.h"
#include "Midi/Players/PlatformMidiManager.h"
#include "Midi/CommonMidiUtils.h"
#include "Midi/Sequence.h"

#include <wx/msgdlg.h>
#include <wx/timer.h>
#include <wx/intl.h>
#include <cstdlib>
#include <cstdio>

#include "camd_player.h"

namespace AriaMaestosa
{
    class CamdMidiManager;

    /* Ends a preview note after its duration, on the main thread. */
    class CamdStopNoteTimer : public wxTimer
    {
        CamdMidiManager* m_owner;
    public:
        CamdStopNoteTimer(CamdMidiManager* owner) : m_owner(owner) { }
        void Notify();
    };

    class CamdMidiManager : public PlatformMidiManager
    {
        bool m_open;
        bool m_told;
        bool m_was_playing;
        amp_song m_map;     // the playing song's tick map (the player has its own copy)
        int m_last_note, m_last_channel;
        CamdStopNoteTimer* m_stop_timer;

        bool ready()
        {
            if (m_open) return true;
            m_open = acp_open() != 0;
            if (not m_open and not m_told)
            {
                m_told = true;
                wxMessageBox(_("Aria Maestosa plays through CAMD: camd.library and a MIDI driver are needed "
                               "(the link goes to the cluster named in ENV:AriaMaestosa/MidiOut, "
                               "else akikoplus.out.0 for AkikoPlus/OpenSynth). "
                               "Songs can still be written and saved as MIDI files."));
            }
            return m_open;
        }

        bool play(Sequence* sequence, bool selection_only, int* startTick)
        {
            if (acp_playing() or not ready()) return false;
            stopNote();
            int songlength = -1, len = -1;
            char* data = NULL;
            allocAsMidiBytes(sequence, selection_only, &songlength, startTick, &data, &len, true);
            if (data == NULL or len <= 0)
            {
                std::fprintf(stderr, "[CAMD] Aria produced no playable SMF (data=%p, length=%d)\n",
                             (void*)data, len);
                return false;
            }
            amp_song song;
            amp_free(&m_map);
            const int song_result = amp_parse((const uint8_t*)data, (uint32_t)len, &song);
            const int map_result = song_result == AMP_OK
                ? amp_parse((const uint8_t*)data, (uint32_t)len, &m_map) : song_result;
            free(data);
            if (song_result != AMP_OK or map_result != AMP_OK)
            {
                std::fprintf(stderr, "[CAMD] SMF parser rejected Aria output (song=%d, map=%d, length=%d)\n",
                             song_result, map_result, len);
                amp_free(&song); amp_free(&m_map); return false;
            }
            m_was_playing = acp_play(&song) != 0;
            if (not m_was_playing)
                std::fprintf(stderr, "[CAMD] player did not start (%lu events, %lu us)\n",
                             (unsigned long)m_map.n, (unsigned long)m_map.end_us);
            return m_was_playing;
        }

    public:
        CamdMidiManager() : m_open(false), m_told(false), m_was_playing(false),
                            m_last_note(-1), m_last_channel(0), m_stop_timer(NULL)
        {
            m_map.ev = NULL; m_map.n = 0; m_map.division = 0; m_map.end_us = 0; m_map.end_tick = 0;
        }
        virtual ~CamdMidiManager() { amp_free(&m_map); }

        virtual bool playSequence(Sequence* sequence, int* startTick) { return play(sequence, false, startTick); }
        virtual bool playSelected(Sequence* sequence, int* startTick) { return play(sequence, true, startTick); }
        virtual bool isPlaying() { return acp_playing() != 0; }
        virtual void stop() { acp_stop(); }

        virtual void exportAudioFile(Sequence* sequence, wxString filepath)
        {
            wxMessageBox(_("Audio export is not in the Amiga version: save the song as a MIDI file."));
        }
        virtual const wxString getAudioExtension() { return wxT(".wav"); }
        virtual const wxString getAudioWildcard() { return wxString(_("WAV file")) + wxT("|*.wav"); }

        virtual int trackPlaybackProgression()
        {
            if (not acp_playing())
            {
                if (m_was_playing)
                {
                    m_was_playing = false;
                    Core::songHasFinishedPlaying();
                }
                return -1;
            }
            return (int)amp_tick_at(&m_map, acp_position_us());
        }
        virtual int getAccurateTick() { return trackPlaybackProgression(); }

        virtual void initMidiPlayer()
        {
            m_stop_timer = new CamdStopNoteTimer(this);
            ready();
        }
        virtual void freeMidiPlayer()
        {
            if (m_stop_timer != NULL) { m_stop_timer->Stop(); delete m_stop_timer; m_stop_timer = NULL; }
            acp_close();
            m_open = false;
        }

        virtual void playNote(int noteNum, int volume, int duration, int channel, int instrument)
        {
            if (not m_open or acp_playing()) return;
            if (m_last_note != -1) stopNote();
            if (channel != 9) acp_send((uint32_t)(0xc0 | channel) << 24 | (uint32_t)(instrument & 0x7f) << 16);
            acp_send((uint32_t)(0x90 | channel) << 24 | (uint32_t)(noteNum & 0x7f) << 16 | (uint32_t)(volume & 0x7f) << 8);
            m_last_note = noteNum;
            m_last_channel = channel;
            if (m_stop_timer != NULL) m_stop_timer->Start(duration, wxTIMER_ONE_SHOT);
        }
        virtual void stopNote()
        {
            if (m_last_note == -1) return;
            acp_send((uint32_t)(0x80 | m_last_channel) << 24 | (uint32_t)(m_last_note & 0x7f) << 16);
            m_last_note = -1;
        }

        virtual wxArrayString getOutputChoices()
        {
            wxArrayString out;
            out.Add(wxString::FromAscii(acp_cluster()));
            return out;
        }
        virtual wxArrayString getInputChoices() { return wxArrayString(); }
    };

    void CamdStopNoteTimer::Notify() { m_owner->stopNote(); }

    class CamdMidiManagerFactory : public PlatformMidiManagerFactory
    {
    public:
        CamdMidiManagerFactory() : PlatformMidiManagerFactory(wxT("CAMD")) { }
        virtual ~CamdMidiManagerFactory() { }
        virtual PlatformMidiManager* newInstance() { return new CamdMidiManager(); }
    };
    CamdMidiManagerFactory g_camd_midi_manager_factory;
}

#endif
