/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001, 2002  Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/*
	Atari MetaDOS CD-ROM functions

	Patrice Mandin
*/

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cdromio.h>
#include <metados.h>

#include "SDL_error.h"
#include "SDL_cdrom.h"
#include "SDL_syscdrom.h"


/* The maximum number of CD-ROM drives we'll detect */
#define MAX_DRIVES	32	

/* Type of CD-ROM drive */
#define DRIVE_TYPE_NOCD	-1
#define DRIVE_TYPE_STANDARD	0
#define DRIVE_TYPE_CDAR	1

typedef struct {
	int	type;					/* Standard, or old CDAR-type CD drive ? */
	unsigned char device[3];	/* Physical device letter + ':' + '\0' */
	metaopen_t	metaopen;		/* Infos on opened drive */
} metados_drive_t;

static metados_drive_t metados_drives[MAX_DRIVES];

/* The system-dependent CD control functions */
static const char *SDL_SYS_CDName(int drive);
static int SDL_SYS_CDOpen(int drive);
static void SDL_SYS_CDClose(SDL_CD *cdrom);
static int SDL_SYS_CDioctl(int id, int command, void *arg);

/* Commands using ioctl() */
static int SDL_SYS_CDGetTOC(SDL_CD *cdrom);
static CDstatus SDL_SYS_CDStatus(SDL_CD *cdrom, int *position);
static int SDL_SYS_CDPlay(SDL_CD *cdrom, int start, int length);
static int SDL_SYS_CDPause(SDL_CD *cdrom);
static int SDL_SYS_CDResume(SDL_CD *cdrom);
static int SDL_SYS_CDStop(SDL_CD *cdrom);
static int SDL_SYS_CDEject(SDL_CD *cdrom);

/* Commands using Meta*() CDAR functions */
static int SDL_SYS_CD_BcdToBinary(int value);
static int SDL_SYS_CDGetTOC_CDAR(SDL_CD *cdrom);
static CDstatus SDL_SYS_CDStatus_CDAR(SDL_CD *cdrom, int *position);

int SDL_SYS_CDInit(void)
{
	metainit_t	metainit={0,0,0,0};
	metaopen_t	metaopen;
	int i, handle;

	Metainit(&metainit);
	if (metainit.version == NULL) {
#ifdef DEBUG_CDROM
		fprintf(stderr, "MetaDOS not installed\n");
#endif
		return -1;
	}

	if (metainit.drives_map == 0) {
#ifdef DEBUG_CDROM
		fprintf(stderr, "No MetaDOS devices present\n");
#endif
		return -1;
	}

	SDL_numcds = 0;
	
	for (i='A'; i<='Z'; i++) {
		metados_drives[SDL_numcds].type = DRIVE_TYPE_NOCD;
		metados_drives[SDL_numcds].device[0] = 0;
		metados_drives[SDL_numcds].device[1] = ':';
		metados_drives[SDL_numcds].device[2] = 0;

		if (metainit.drives_map & (1<<(i-'A'))) {
			handle = Metaopen(i, &metaopen);
			if (handle == 0) {

				if ( (metaopen.name[0]=='C') && (metaopen.name[1]=='D') &&
					(metaopen.name[2]=='A') && (metaopen.name[3]=='R')) {
					/* Drive compatible with CDAR */
					metados_drives[SDL_numcds].type = DRIVE_TYPE_CDAR;
					metados_drives[SDL_numcds].device[0] = i;
					++SDL_numcds;
				} else {
					/* Check for a CD-ROM device */
					if ((Metastatus(i, NULL) & 0x7fff) >= 0) {
						/* Drive compatible with new ioctl functions */
						metados_drives[SDL_numcds].type = DRIVE_TYPE_STANDARD;
						metados_drives[SDL_numcds].device[0] = i;
						++SDL_numcds;
					}
				}

				Metaclose(i);
			}
		}
	}

	/* Fill in our driver capabilities */
	SDL_CDcaps.Name = SDL_SYS_CDName;
	SDL_CDcaps.Open = SDL_SYS_CDOpen;
	SDL_CDcaps.Close = SDL_SYS_CDClose;

	SDL_CDcaps.GetTOC = SDL_SYS_CDGetTOC;
	SDL_CDcaps.Status = SDL_SYS_CDStatus;
	SDL_CDcaps.Play = SDL_SYS_CDPlay;
	SDL_CDcaps.Pause = SDL_SYS_CDPause;
	SDL_CDcaps.Resume = SDL_SYS_CDResume;
	SDL_CDcaps.Stop = SDL_SYS_CDStop;
	SDL_CDcaps.Eject = SDL_SYS_CDEject;

	return 0;
}

void SDL_SYS_CDQuit(void)
{
	SDL_numcds = 0;
}

static const char *SDL_SYS_CDName(int drive)
{
	return(metados_drives[drive].device);
}

static int SDL_SYS_CDOpen(int drive)
{
	int handle;

	handle = Metaopen(metados_drives[drive].device[0], &(metados_drives[drive].metaopen));
	if (handle == 0) {
		return drive;
	}

	return -1;
}

static void SDL_SYS_CDClose(SDL_CD *cdrom)
{
	Metaclose(metados_drives[cdrom->id].device[0]);
}

static int SDL_SYS_CDioctl(int id, int command, void *arg)
{
	int retval;

	retval = Metaioctl(metados_drives[id].device[0], METADOS_IOCTL_MAGIC, command, arg);
	if ( retval < 0 ) {
		SDL_SetError("ioctl() error: %s", strerror(errno));
	}
	return(retval);
}

static int SDL_SYS_CD_BcdToBinary(int value)
{
	int tmp;
	
	tmp = (value>>4) & 0xf;	
	return (tmp*10)+(value & 0xf);
}

static int SDL_SYS_CDGetTOC_CDAR(SDL_CD *cdrom)
{
	int errorcode, i, minute, second, frame;
	metatocentry_t *toc_entries;
	metadiscinfo_t	disc_info;

	/* First, read disc info */
	errorcode = Metadiscinfo(metados_drives[cdrom->id].device[0], &disc_info);
	if (errorcode<0) {
#ifdef DEBUG_CDROM
		fprintf(stderr, "Can not read disc info\n");
#endif
		return -1;
	}
	
	cdrom->numtracks = disc_info.last - disc_info.first + 1;

	/* Then read toc entries for tracks */
	toc_entries = (metatocentry_t *)malloc(100*sizeof(metatocentry_t));
	if (toc_entries == NULL) {
#ifdef DEBUG_CDROM
		fprintf(stderr, "Can not allocate memory for TOC entries\n");
#endif
		return -1;
	}

	errorcode = Metagettoc(metados_drives[cdrom->id].device[0], 0, toc_entries);
	if (errorcode<0) {
#ifdef DEBUG_CDROM
		fprintf(stderr, "Can not read TOC\n");
#endif
		free(toc_entries);
		return -1;
	}

	i=0;
	for (;;) {
		if ((toc_entries[i].track==0) && (toc_entries[i].minute==0) && 
			(toc_entries[i].second==0) && (toc_entries[i].frame==0)
			) {
			break;
		}

		if (toc_entries[i].track == CDROM_LEADOUT_CDAR) {
			cdrom->track[i].id = CDROM_LEADOUT;
		} else {
			cdrom->track[i].id = toc_entries[i].track;
		}
		if (disc_info.disctype == 0) {
			cdrom->track[i].type = SDL_AUDIO_TRACK;
		} else {
			cdrom->track[i].type = SDL_DATA_TRACK;
		}
		minute = SDL_SYS_CD_BcdToBinary(toc_entries[i].minute);
		second = SDL_SYS_CD_BcdToBinary(toc_entries[i].second);
		frame = SDL_SYS_CD_BcdToBinary(toc_entries[i].frame);
		cdrom->track[i].offset = MSF_TO_FRAMES(minute, second, frame);

		if ( i > 0 ) {
			cdrom->track[i-1].length = cdrom->track[i].offset - cdrom->track[i-1].offset;
		}

		++i;
	}

	free(toc_entries);
	return 0;
}

static int SDL_SYS_CDGetTOC(SDL_CD *cdrom)
{
	int i,okay;
	struct cdrom_tochdr toc;
	struct cdrom_tocentry entry;

	/* CDAR compatible drive ? */
	if (metados_drives[cdrom->id].type == DRIVE_TYPE_CDAR) {
		return SDL_SYS_CDGetTOC_CDAR(cdrom);
	}

	/* Use standard ioctl() */	
	if (SDL_SYS_CDioctl(cdrom->id, CDROMREADTOCHDR, &toc)<0) {
		return -1;
	}

	cdrom->numtracks = toc.cdth_trk1-toc.cdth_trk0+1;
	if ( cdrom->numtracks > SDL_MAX_TRACKS ) {
		cdrom->numtracks = SDL_MAX_TRACKS;
	}

	/* Read all the track TOC entries */
	okay=1;
	for ( i=0; i<=cdrom->numtracks; ++i ) {
		if ( i == cdrom->numtracks ) {
			cdrom->track[i].id = CDROM_LEADOUT;
		} else {
			cdrom->track[i].id = toc.cdth_trk0+i;
		}
		entry.cdte_track = cdrom->track[i].id;
		entry.cdte_format = CDROM_MSF;
		if ( SDL_SYS_CDioctl(cdrom->id, CDROMREADTOCENTRY, &entry) < 0 ) {
			okay=0;
			break;
		} else {
			if ( entry.cdte_ctrl & CDROM_DATA_TRACK ) {
				cdrom->track[i].type = SDL_DATA_TRACK;
			} else {
				cdrom->track[i].type = SDL_AUDIO_TRACK;
			}
			cdrom->track[i].offset = MSF_TO_FRAMES(
				entry.cdte_addr.msf.minute,
				entry.cdte_addr.msf.second,
				entry.cdte_addr.msf.frame);
				cdrom->track[i].length = 0;
			if ( i > 0 ) {
				cdrom->track[i-1].length = cdrom->track[i].offset-cdrom->track[i-1].offset;
			}
		}
	}

	return(okay ? 0 : -1);
}

/* Get CD-ROM status */
static CDstatus SDL_SYS_CDStatus_CDAR(SDL_CD *cdrom, int *position)
{
	return CD_ERROR;
}

static CDstatus SDL_SYS_CDStatus(SDL_CD *cdrom, int *position)
{
	CDstatus status;
	struct cdrom_tochdr toc;
	struct cdrom_subchnl info;

	/* CDAR compatible drive ? */
	if (metados_drives[cdrom->id].type == DRIVE_TYPE_CDAR) {
		return SDL_SYS_CDStatus_CDAR(cdrom, position);
	}

	/* Standard ioctl */
	info.cdsc_format = CDROM_MSF;
	if ( SDL_SYS_CDioctl(cdrom->id, CDROMSUBCHNL, &info) < 0 ) {
		status = CD_TRAYEMPTY;
	} else {
		switch (info.cdsc_audiostatus) {
			case CDROM_AUDIO_INVALID:
			case CDROM_AUDIO_NO_STATUS:
				/* Try to determine if there's a CD available */
				if (SDL_SYS_CDioctl(cdrom->id, CDROMREADTOCHDR, &toc)==0) {
					status = CD_STOPPED;
				} else {
					status = CD_TRAYEMPTY;
				}
				break;
			case CDROM_AUDIO_COMPLETED:
				status = CD_STOPPED;
				break;
			case CDROM_AUDIO_PLAY:
				status = CD_PLAYING;
				break;
			case CDROM_AUDIO_PAUSED:
				/* Workaround buggy CD-ROM drive */
				if ( info.cdsc_trk == CDROM_LEADOUT ) {
					status = CD_STOPPED;
				} else {
					status = CD_PAUSED;
				}
				break;
			default:
				status = CD_ERROR;
				break;
		}
	}
	if ( position ) {
		if ( status == CD_PLAYING || (status == CD_PAUSED) ) {
			*position = MSF_TO_FRAMES(
					info.cdsc_absaddr.msf.minute,
					info.cdsc_absaddr.msf.second,
					info.cdsc_absaddr.msf.frame);
		} else {
			*position = 0;
		}
	}
	return(status);
}

/* Start play */
static int SDL_SYS_CDPlay_CDAR(SDL_CD *cdrom, int start, int length)
{
	struct cdrom_msf playtime;

	FRAMES_TO_MSF(start,
	   &playtime.cdmsf_min0, &playtime.cdmsf_sec0, &playtime.cdmsf_frame0);
	FRAMES_TO_MSF(start+length,
	   &playtime.cdmsf_min1, &playtime.cdmsf_sec1, &playtime.cdmsf_frame1);

	return Metasetsongtime(metados_drives[cdrom->id].device[0], 0,
		(playtime.cdmsf_min0<<16)|(playtime.cdmsf_sec0<<8)|(playtime.cdmsf_frame0),
		(playtime.cdmsf_min1<<16)|(playtime.cdmsf_sec1<<8)|(playtime.cdmsf_frame1)
	);
}

static int SDL_SYS_CDPlay(SDL_CD *cdrom, int start, int length)
{
	struct cdrom_msf playtime;

	/* CDAR compatible drive ? */
	if (metados_drives[cdrom->id].type == DRIVE_TYPE_CDAR) {
		return SDL_SYS_CDPlay_CDAR(cdrom, start, length);
	}

	FRAMES_TO_MSF(start,
	   &playtime.cdmsf_min0, &playtime.cdmsf_sec0, &playtime.cdmsf_frame0);
	FRAMES_TO_MSF(start+length,
	   &playtime.cdmsf_min1, &playtime.cdmsf_sec1, &playtime.cdmsf_frame1);
#ifdef DEBUG_CDROM
  fprintf(stderr, "Trying to play from %d:%d:%d to %d:%d:%d\n",
	playtime.cdmsf_min0, playtime.cdmsf_sec0, playtime.cdmsf_frame0,
	playtime.cdmsf_min1, playtime.cdmsf_sec1, playtime.cdmsf_frame1);
#endif
	return(SDL_SYS_CDioctl(cdrom->id, CDROMPLAYMSF, &playtime));
}

/* Pause play */
static int SDL_SYS_CDPause(SDL_CD *cdrom)
{
	/* CDAR compatible drive ? */
	if (metados_drives[cdrom->id].type == DRIVE_TYPE_CDAR) {
		return -1;
	}

	return(SDL_SYS_CDioctl(cdrom->id, CDROMPAUSE, 0));
}

/* Resume play */
static int SDL_SYS_CDResume(SDL_CD *cdrom)
{
	/* CDAR compatible drive ? */
	if (metados_drives[cdrom->id].type == DRIVE_TYPE_CDAR) {
		return -1;
	}

	return(SDL_SYS_CDioctl(cdrom->id, CDROMRESUME, 0));
}

/* Stop play */
static int SDL_SYS_CDStop(SDL_CD *cdrom)
{
	/* CDAR compatible drive ? */
	if (metados_drives[cdrom->id].type == DRIVE_TYPE_CDAR) {
		return Metastopaudio(metados_drives[cdrom->id].device[0]);
	}

	return(SDL_SYS_CDioctl(cdrom->id, CDROMSTOP, 0));
}

/* Eject the CD-ROM */
static int SDL_SYS_CDEject(SDL_CD *cdrom)
{
	/* CDAR compatible drive ? */
	if (metados_drives[cdrom->id].type == DRIVE_TYPE_CDAR) {
		return -1;
	}

	return(SDL_SYS_CDioctl(cdrom->id, CDROMEJECT, 0));
}
