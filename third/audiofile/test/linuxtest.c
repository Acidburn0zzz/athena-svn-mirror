/*
	Audio File Library

	Copyright 1998-1999, Michael Pruett <michael@68k.org>

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.

	This program is distributed in the hope that it will be
	useful, but WITHOUT ANY WARRANTY; without even the implied
	warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
	PURPOSE.  See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public
	License along with this program; if not, write to the Free
	Software Foundation, Inc., 59 Temple Place, Suite 330, Boston,
	MA 02111-1307, USA.
*/

/*
	linuxtest.c

	This file plays a 16-bit, 44.1 kHz monophonic or stereophonic
	audio file through a PC sound card on a Linux system.  This file
	will not compile under any operating system that does not support
	the Open Sound System API.
*/

#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <linux/soundcard.h>
#include <audiofile.h>

void setupdsp (int audiofd, int channelCount);
void usage (void);

int main (int argc, char **argv)
{
	AFfilehandle	file;
	AFframecount	frameCount;
	int		sampleFormat, sampleWidth, channelCount, frameSize;
	void		*buffer;
	int		audiofd;

	if (argc != 2)
		usage();

	file = afOpenFile(argv[1], "r", NULL);
	frameCount = afGetFrameCount(file, AF_DEFAULT_TRACK);
	printf("frame count: %d\n", (int)frameCount);

	channelCount = afGetChannels(file, AF_DEFAULT_TRACK);
	afGetSampleFormat(file, AF_DEFAULT_TRACK, &sampleFormat, &sampleWidth);

	frameSize = afGetFrameSize(file, AF_DEFAULT_TRACK, 1);

	printf("sample format: %d, sample width: %d, channels: %d\n",
		sampleFormat, sampleWidth, channelCount);

	if ((sampleFormat != AF_SAMPFMT_TWOSCOMP) &&
		(sampleFormat != AF_SAMPFMT_UNSIGNED))
	{
		printf("The audio file must contain integer data in two's complement or unsigned format.\n");
		exit(-1);
	}

	if ((sampleWidth != 16) || (channelCount > 2))
	{
		printf("The audio file must be of a 16-bit monophonic or stereophonic format.\n");
		exit(-1);
	}

	buffer = malloc(frameCount * frameSize);
	afReadFrames(file, AF_DEFAULT_TRACK, buffer, frameCount);

	audiofd = open("/dev/dsp", O_WRONLY);
	if (audiofd < 0)
	{
		perror("open");
		exit(-1);
	}

	setupdsp(audiofd, channelCount);

	write(audiofd, buffer, frameCount * frameSize);
	close(audiofd);
	free(buffer);

	return 0;
}

void setupdsp (int audiofd, int channelCount)
{
	int	format, frequency, channels;

	format = AFMT_S16_NE;
	if (ioctl(audiofd, SNDCTL_DSP_SETFMT, &format) == -1)
	{
		perror("set format");
		exit(-1);
	}

	if (format != AFMT_S16_NE)
	{
		fprintf(stderr, "format not correct.\n");
		exit(-1);
	}

	channels = channelCount;
	if (ioctl(audiofd, SNDCTL_DSP_CHANNELS, &channels) == -1)
	{
		perror("set channels");
		exit(-1);
	}

	frequency = 44100;
	if (ioctl(audiofd, SNDCTL_DSP_SPEED, &frequency) == -1)
	{
		perror("set frequency");
		exit(-1);
	}
}

void usage (void)
{
	fprintf(stderr, "usage: linuxtest file\n");
	fprintf(stderr,
		"where file refers to a 16-bit monophonic or stereophonic 44.1 kHz audio file\n");
	exit(-1);
}
