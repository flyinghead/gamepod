#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <alsa/global.h>
#include <alsa/input.h>
#include <alsa/output.h>
#include <alsa/conf.h>
#include <alsa/control.h>
#include <alsa/pcm.h>
#include <alsa/mixer.h>

static const char *card = "default";	// TODO arg?
extern const char *alsa_mixer;

static snd_mixer_elem_t* getMixer(snd_mixer_t *handle) {
    snd_mixer_selem_id_t *sid;

    snd_mixer_attach(handle, card);
    snd_mixer_selem_register(handle, NULL, NULL);
    snd_mixer_load(handle);

    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_index(sid, 0);
    snd_mixer_selem_id_set_name(sid, alsa_mixer);
    snd_mixer_elem_t* elem = snd_mixer_find_selem(handle, sid);

    if (elem == NULL)
	fprintf(stderr, "Can't find ALSA device %s\n", alsa_mixer);

    return elem;
}

void setAlsaMasterVolume(int volume)
{
    long min, max;
    snd_mixer_t *handle;

    snd_mixer_open(&handle, 0);
    snd_mixer_elem_t* elem = getMixer(handle);
    if (elem != NULL) {
	snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	snd_mixer_selem_set_playback_volume_all(elem, min + volume * (max - min) / 100);
    }

    snd_mixer_close(handle);
}

int getAlsaMasterVolume() {
    long min, max;
    snd_mixer_t *handle;

    snd_mixer_open(&handle, 0);
    snd_mixer_elem_t* elem = getMixer(handle);
    if (elem == NULL) {
	snd_mixer_close(handle);
	return 0;
    }

    snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    long volume;
    snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT, &volume);

    snd_mixer_close(handle);

    return (volume - min) * 100 / (max - min);
}
