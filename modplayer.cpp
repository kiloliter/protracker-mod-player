#include <iostream>
#include "SDL/SDL.h"
#include "SDL/SDL_audio.h"
using namespace std;
extern void audio_callback (void *unused, Uint8 * stream, int length);
class Protracker_module;
int mixer_frequency = 44100;

class Sample
{
      public:
	char name[26];
	long int sample_length;
	int volume;
	bool loop;
	long int loop_start;
	long int loop_end;
	char *sample_data;
};

class Row
{
      public:
	int period;
	int note;
	int octave;
	int sample;
	int effect;
	int effect_data;
};

class Pattern
{
      public:
	Row rows[4][64];
};

class Channel
{
      public:
	Channel ();
	Sint16 get_frame ();
	void update_tick ();
	void update_row (Row * row);
	Sample *current_sample;
	double sample_position;
	int current_period;
	int temp_period;
	int current_frequency;
	int volume;
	int portamento_up;
	int portamento_down;
	int tone_portamento_speed;
	int tone_portamento_period;
	int volume_slide_up;
	int volume_slide_down;
	int vibrato_speed;
	int vibrato_depth;
	int vibrato_type;
	int vibrato_position;
	int vibrato_data;
	int tremolo_speed;
	int tremolo_depth;
	int tremolo_type;
	int tremolo_position;
	int tremolo_data;
	int arpeggio_1;
	int arpeggio_2;
	int arpeggio_counter;
};

Channel::Channel ()
{
	current_sample = NULL;
	sample_position = 0;
	current_period = 0;
	current_frequency = 0;
	volume = 64;
	portamento_up = 0;
	portamento_down = 0;
	tone_portamento_speed = 0;
	tone_portamento_period = 0;
	volume_slide_up = 0;
	volume_slide_down = 0;
	vibrato_speed = 0;
	vibrato_depth = 0;
	vibrato_type = 0;
	vibrato_position = 0;
	vibrato_data = 0;
	tremolo_speed = 0;
	tremolo_depth = 0;
	tremolo_type = 0;
	tremolo_position = 0;
	tremolo_data = 0;
	arpeggio_1 = -1;
	arpeggio_2 = -1;
	arpeggio_counter = 0;
}

const int16_t sine_table[64] = {
	0, 24, 49, 74, 97, 120, 141, 161,
	180, 197, 212, 224, 235, 244, 250, 253,
	255, 253, 250, 244, 235, 224, 212, 197,
	180, 161, 141, 120, 97, 74, 49, 24,
	-0, -24, -49, -74, -97, -120, -141, -161,
	-180, -197, -212, -224, -235, -244, -250, -253,
	-255, -253, -250, -244, -235, -224, -212, -197,
	-180, -161, -141, -120, -97, -74, -49, -24
};

void
Channel::update_tick ()
{
	if (current_sample == NULL || current_period == 0)
		return;
	// portamento down
	if (current_period != 0 && portamento_down != 0)
	{
		current_period += portamento_down;
	}
	// portamento up
	if (current_period != 0 && portamento_up != 0)
	{
		current_period -= portamento_up;
	}
	if (tone_portamento_speed != 0)
	{
		if (current_period > tone_portamento_period)
		{
			current_period -= tone_portamento_speed;
			if (current_period < tone_portamento_period)
			{
				current_period = tone_portamento_period;
			}
		}
		if (current_period < tone_portamento_period)
		{
			current_period += tone_portamento_speed;
			if (current_period > tone_portamento_period)
			{
				current_period = tone_portamento_period;
			}
		}
	}
	if (volume_slide_up != 0)
	{
		volume += volume_slide_up;
		if (volume > 64)
			volume = 64;
	}
	else if (volume_slide_down != 0)
	{
		volume -= volume_slide_down;
		if (volume < 0)
			volume = 0;
	}
	if (vibrato_depth != 0)
	{
		vibrato_data = sine_table[vibrato_position];
		vibrato_position += vibrato_speed;
		vibrato_position = vibrato_position % 64;
		vibrato_data = vibrato_data * vibrato_depth / 128;
	}
	if (tremolo_depth != 0)
	{
		tremolo_data = sine_table[tremolo_position];
		tremolo_position += tremolo_speed;
		tremolo_position = tremolo_position % 64;
		tremolo_data = tremolo_data * tremolo_depth / 128;
	}
	temp_period = current_period;
	// arpeggio
	if (arpeggio_1 > -1)
	{
	}
	current_frequency = 3546895 / (temp_period + vibrato_data);
}

Sint16 Channel::get_frame ()
{
	Sint16
		output = 0;
	if (current_sample != NULL)
	{
		if (int (sample_position) < current_sample->sample_length)
		{
			int
				output_volume = volume + tremolo_data;
			if (output_volume > 64)
			{
				output_volume = 64;
			}
			output = current_sample->sample_data[int (sample_position)] *
				63.0 *
				output_volume /
				64;
			sample_position += current_frequency / (float) mixer_frequency;
			if (current_sample->loop == true && sample_position > current_sample->loop_end)
			{
				sample_position = current_sample->loop_start;
			}
		}
	}
	return output;
}

class
	Protracker_module
{
	int
		current_row;
	int
		order_location;
	Channel
		channels[4];
	int
		speed_counter;
	int
		tempo_counter;
	int
		order_list[128];
	char
		song_name[26];
	int
		order_list_length;
	int
		pattern_count;
	int
		instrument_count;
	Pattern *
		patterns;
	void
	update_row ();
      public:
	Protracker_module ();
	int
	load (const char *filename);
	Sint16
	get_frame ();
	Sample *
		samples;
	int
		current_speed;
	int
		current_tempo;
	int
		pattern_jump_position;
};

extern Protracker_module
	module;

void
Channel::update_row (Row * row)
{
	// update sample
	if (row->period != 0 && row->effect != 0x3)
	{			// only update the period if the note is not part of a note slide
		sample_position = 0;
		current_period = row->period;
	}
	if (row->sample != -1)
	{
		current_sample = &module.samples[row->sample];
		volume = current_sample->volume;
	}
	portamento_up = 0;
	portamento_down = 0;
	arpeggio_1 = -1;
	arpeggio_2 = -1;
	// effects

	// arpeggio
	if (row->effect == 0x0)
	{
		arpeggio_1 = 3;
	}

	// portamento up
	if (row->effect == 0x1)
	{
		portamento_up = row->effect_data;
	}
	// portamento down
	if (row->effect == 0x2)
	{
		portamento_down = row->effect_data;
	}
	// note portamento
	if (row->effect == 0x3)
	{
		if (row->effect_data != 0)
			tone_portamento_speed = row->effect_data;
		if (row->period != 0)
			tone_portamento_period = row->period;
	}
	if (row->effect != 0x3 && row->effect != 0x5)
	{			// figure out if we should turn off note portamento
		tone_portamento_speed = 0;
	}

	// vibrato
	if (row->effect == 0x4)
	{
		if (row->effect_data & 15 != 0)
		{
			vibrato_depth = row->effect_data & 15;
			vibrato_speed = row->effect_data >> 4;
		}
	}
	if (row->effect != 0x4 && row->effect != 0x6)
	{			// figure out if we should turn off vibrato
		vibrato_depth = 0;
		vibrato_speed = 0;
		vibrato_data = 0;
	}

	// volume slide
	// volume slide + continue note portamento
	if (row->effect == 0xa || row->effect == 0x5 || row->effect == 0x6)
	{
		volume_slide_down = row->effect_data & 15;
		volume_slide_up = row->effect_data >> 4;
	}
	else
	{
		volume_slide_down = 0;
		volume_slide_up = 0;
	}

	// tremolo
	if (row->effect == 0x7)
	{
		if (row->effect_data & 15 != 0)
		{
			tremolo_depth = row->effect_data & 15;
			tremolo_speed = row->effect_data >> 4;
		}
	}
	if (row->effect != 0x7)
	{
		tremolo_speed = 0;
		tremolo_depth = 0;
		tremolo_data = 0;
	}
	// TODO fine panning
	// sample offset
	if (row->effect == 0x9)
	{
		sample_position += row->effect_data * 256;
	}
	// set channel volume
	if (row->effect == 0xc)
	{
		volume = row->effect_data;
	}
	// pattern break
	if (row->effect == 0xd)
	{
		module.pattern_jump_position = (10 * (row->effect_data >> 4)) + (row->effect_data & 15);
	}
	// e commands
	else if (row->effect == 0xe)
	{
		switch (row->effect_data >> 4)
		{
			// fine slide up
		case 0x1:
			current_period -= row->effect_data & 15;
			break;
			// fine slide down
		case 0x2:
			current_period += row->effect_data & 15;
			break;
			// fine volume slide up
		case 0xa:
			volume += row->effect_data & 15;
			if (volume > 64)
				volume = 64;
			break;
			// fine volume slide down
		case 0xb:
			volume -= row->effect_data & 15;
			if (volume < 0)
				volume = 0;
			break;
		}
	}
	else if (row->effect == 0xf)
	{
		if (row->effect_data > 0x20)
			// tempo
			module.current_tempo = row->effect_data;
		else
			// speed
			module.current_speed = row->effect_data;
	}


}

Protracker_module::Protracker_module ()
{
	current_speed = 6;
	current_tempo = 125;
	speed_counter = 0;
	tempo_counter = 0;
	current_row = 0;
	order_location = 0;
	pattern_jump_position = -1;
}

Protracker_module module;

void
Protracker_module::update_row ()
{
	for (int channel = 0; channel < 4; channel++)
	{
		channels[channel].update_row (&patterns[order_list[order_location]].rows[channel][current_row]);
	}
}

Sint16 Protracker_module::get_frame ()
{
	Sint16
		output = 0;
	for (int channel = 0; channel < 4; channel++)
	{
		output += channels[channel].get_frame ();
	}
	tempo_counter++;
	// next tick
	if (tempo_counter >= mixer_frequency * 2.5 / current_tempo)
	{
		tempo_counter = 0;
		speed_counter++;
		if (speed_counter >= current_speed)
		{
			speed_counter = 0;
			if (pattern_jump_position != -1)
			{
				current_row = pattern_jump_position;
				pattern_jump_position = -1;
				order_location++;
				if (order_location >= order_list_length)
				{
					order_location = 0;
				}
			}
			else
			{
				current_row++;
				if (current_row >= 64)
				{
					current_row = 0;
					order_location++;
					if (order_location >= order_list_length)
					{
						order_location = 0;
					}
				}
			}
			update_row ();
			return output;
		}
		// take care of effects
		for (int channel = 0; channel < 4; channel++)
		{
			channels[channel].update_tick ();
		}
	}
	return output;
}

void
audio_callback (void *unused, Uint8 * stream, int length)
{
	Sint16 *
		_stream = (Sint16 *) stream;
	int
		_length = length / 2;
	for (int i = 0; i < _length; i += 2)
	{
		_stream[i + 1] = _stream[i] = module.get_frame ();
	}
}

unsigned long int
get_int_from_buffer (unsigned char *buffer)
{
	return buffer[0] | buffer[1] << 8;
}

unsigned long int
get_big_endian_int_from_word (unsigned char *buffer)
{
	return buffer[1] | buffer[0] << 8;
}

int
protracker_note_periods[3][12] = { {856, 808, 762, 720, 678, 640, 604, 570, 538, 508, 480, 453},
{428, 404, 381, 360, 339, 320, 302, 285, 269, 254, 240, 226},
{214, 202, 190, 180, 170, 160, 151, 143, 135, 127, 120, 113}
};

const char *
note_strings[12] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

int
Protracker_module::load (const char *filename)
{
	unsigned char buffer[4];
	FILE *ptr;

	ptr = fopen (filename, "rb");

	// song name
	fread (song_name, 20, 1, ptr);
	cout << "song name: " << song_name << '\n';

	// load samples (but not sample data itself)
	cout << "samples:\n";
	samples = new Sample[31];
	for (int i = 0; i < 31; i++)
	{
		// sample name
		fseek (ptr, i * 30 + 20, SEEK_SET);
		fread (samples[i].name, 22, 1, ptr);
		cout << i + 1 << ": ";
		cout << samples[i].name << '\n';

		// sample length
		fread (buffer, 2, 1, ptr);
		samples[i].sample_length = get_big_endian_int_from_word (buffer) * 2;

		// sample finetune
		fread (buffer, 1, 1, ptr);

		// sample volume
		fread (buffer, 1, 1, ptr);
		samples[i].volume = (int) buffer[0];

		// loop start
		fread (buffer, 2, 1, ptr);
		samples[i].loop_start = get_big_endian_int_from_word (buffer) * 2;

		// loop end
		fread (buffer, 2, 1, ptr);
		int loop_length = get_big_endian_int_from_word (buffer) * 2;
		// loop?
		if (loop_length > 2)
			samples[i].loop = true;
		samples[i].loop_end = samples[i].loop_start + loop_length;

	}
	// order list length
	fseek (ptr, 950, SEEK_SET);
	fread (buffer, 1, 1, ptr);
	order_list_length = (int) buffer[0];

	// song end jump position TODO
	fread (buffer, 1, 1, ptr);

	// order list
	pattern_count = 0;
	for (int i = 0; i < order_list_length; i++)
	{
		fread (buffer, 1, 1, ptr);
		int tmp = (int) buffer[0];
		if (tmp > pattern_count)
			pattern_count = tmp;
		order_list[i] = tmp;
	}
	pattern_count++;

	fseek (ptr, 1080, SEEK_SET);
	// mod file format tag
	fread (buffer, 4, 1, ptr);
	cout << "file format tag: " << buffer << '\n';

	// pattern data
	patterns = new Pattern[pattern_count];
	for (int pattern = 0; pattern < pattern_count; pattern++)
	{
		for (int row = 0; row < 64; row++)
		{
			for (int channel = 0; channel < 4; channel++)
			{
				fread (buffer, 4, 1, ptr);
				// sample
				patterns[pattern].rows[channel][row].sample = (buffer[0] & 240 | buffer[2] >> 4) - 1;

				// note
				unsigned short int tmp = buffer[0] & 15;
				patterns[pattern].rows[channel][row].period = tmp << 8 | buffer[1];
				patterns[pattern].rows[channel][row].octave = 0;
				patterns[pattern].rows[channel][row].note = 0;
				for (int octave = 0; octave < 2; octave++)
				{
					for (int note = 0; note < 12; note++)
					{
						if (patterns[pattern].rows[channel][row].period == protracker_note_periods[octave][note])
						{
							patterns[pattern].rows[channel][row].octave = octave;
							patterns[pattern].rows[channel][row].note = note;
						}
					}
				}

				// effect
				tmp = buffer[2] & 15;
				patterns[pattern].rows[channel][row].effect = (int) tmp;
				// effect data
				patterns[pattern].rows[channel][row].effect_data = (int) buffer[3];
			}
		}
	}
	fseek (ptr, 1084 + 1024 * pattern_count, SEEK_SET);
	// sample data
	for (int i = 0; i < 31; i++)
	{
		samples[i].sample_data = new char[samples[i].sample_length];
		fread (samples[i].sample_data, samples[i].sample_length, 1, ptr);
	}
	update_row ();
	return 0;
}

int
main (int argc, char **argv)
{
	module.load (argv[1]);
	SDL_AudioSpec fmt;

	fmt.freq = mixer_frequency;
	fmt.format = AUDIO_S16;
	fmt.channels = 2;
	fmt.samples = 512;
	fmt.callback = audio_callback;
	fmt.userdata = NULL;

	if (SDL_OpenAudio (&fmt, NULL) < 0)
	{
		fprintf (stderr, "Unable to open audio: %s\n", SDL_GetError ());
		exit (1);
	}
	SDL_PauseAudio (0);
	while (1)
		SDL_Delay (1000);

	SDL_CloseAudio ();
}
