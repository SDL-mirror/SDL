/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997, 1998, 1999, 2000, 2001  Sam Lantinga

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
    slouken@devolution.com
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id$";
#endif

/* Functions for system-level CD-ROM audio control */

#include <sys/types.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/cdrom.h>
#include <sys/dcmd_cam.h>


#include "SDL_error.h"
#include "SDL_cdrom.h"
#include "SDL_syscdrom.h"


/* The maximum number of CD-ROM drives we'll detect */
#define MAX_DRIVES	16	

/* A list of available CD-ROM drives */
static char *SDL_cdlist[MAX_DRIVES];
static dev_t SDL_cdmode[MAX_DRIVES];

/* The system-dependent CD control functions */
static const char *SDL_SYS_CDName(int drive);
static int SDL_SYS_CDOpen(int drive);
static int SDL_SYS_CDGetTOC(SDL_CD *cdrom);
static CDstatus SDL_SYS_CDStatus(SDL_CD *cdrom, int *position);
static int SDL_SYS_CDPlay(SDL_CD *cdrom, int start, int length);
static int SDL_SYS_CDPause(SDL_CD *cdrom);
static int SDL_SYS_CDResume(SDL_CD *cdrom);
static int SDL_SYS_CDStop(SDL_CD *cdrom);
static int SDL_SYS_CDEject(SDL_CD *cdrom);
static void SDL_SYS_CDClose(SDL_CD *cdrom);

/* Some ioctl() errno values which occur when the tray is empty */
#define ERRNO_TRAYEMPTY(errno)	\
	((errno == EIO) || (errno == ENOENT) || (errno == EINVAL))

/* Check a drive to see if it is a CD-ROM */
static int CheckDrive(char *drive, char *mnttype, struct stat *stbuf)
{
	int is_cd, cdfd;
	cdrom_subch_data_t info;

	/* If it doesn't exist, return -1 */
	if ( stat(drive, stbuf) < 0 ) {
		return(-1);
	}

	/* If it does exist, verify that it's an available CD-ROM */
	is_cd = 0;
	if ( S_ISCHR(stbuf->st_mode) || S_ISBLK(stbuf->st_mode) ) {
		cdfd = open(drive, (O_RDONLY|O_EXCL|O_NONBLOCK), 0);
		if ( cdfd >= 0 ) {
			info.subch_command.data_format = CDROM_MSF;
			/* Under Linux, EIO occurs when a disk is not present.
			 */
			if ((devctl(cdfd,DCMD_CAM_CDROMSUBCHNL,&info,sizeof(info),0) == 0) ||
						ERRNO_TRAYEMPTY(errno) )
			{
				is_cd = 1;
			}
			close(cdfd);
		}
	}
	return(is_cd);
}

/* Add a CD-ROM drive to our list of valid drives */
static void AddDrive(char *drive, struct stat *stbuf)
{
	int i;

	if ( SDL_numcds < MAX_DRIVES ) {
		/* Check to make sure it's not already in our list.
	 	   This can happen when we see a drive via symbolic link.
		 */
		for ( i=0; i<SDL_numcds; ++i ) {
			if ( stbuf->st_rdev == SDL_cdmode[i] ) {
#ifdef DEBUG_CDROM
  fprintf(stderr, "Duplicate drive detected: %s == %s\n", drive, SDL_cdlist[i]);
#endif
				return;
			}
		}

		/* Add this drive to our list */
		i = SDL_numcds;
		SDL_cdlist[i] = (char *)malloc(strlen(drive)+1);
		if ( SDL_cdlist[i] == NULL ) {
			SDL_OutOfMemory();
			return;
		}
		strcpy(SDL_cdlist[i], drive);
		SDL_cdmode[i] = stbuf->st_rdev;
		++SDL_numcds;
#ifdef DEBUG_CDROM
  fprintf(stderr, "Added CD-ROM drive: %s\n", drive);
#endif
	}
}

int  SDL_SYS_CDInit(void)
{
	/* checklist: /dev/cdrom, /dev/hd?, /dev/scd? /dev/sr? */
	static char *checklist[] = {
		"cdrom", "?0 cd?", "?1 cd?", "?a hd?", "?0 scd?", "?0 sr?", NULL
	};
	char *SDLcdrom;
	int i, j, exists;
	char drive[32];
	struct stat stbuf;

	/* Fill in our driver capabilities */
	SDL_CDcaps.Name = SDL_SYS_CDName;
	SDL_CDcaps.Open = SDL_SYS_CDOpen;
	SDL_CDcaps.GetTOC = SDL_SYS_CDGetTOC;
	SDL_CDcaps.Status = SDL_SYS_CDStatus;
	SDL_CDcaps.Play = SDL_SYS_CDPlay;
	SDL_CDcaps.Pause = SDL_SYS_CDPause;
	SDL_CDcaps.Resume = SDL_SYS_CDResume;
	SDL_CDcaps.Stop = SDL_SYS_CDStop;
	SDL_CDcaps.Eject = SDL_SYS_CDEject;
	SDL_CDcaps.Close = SDL_SYS_CDClose;

	/* Look in the environment for our CD-ROM drive list */
	SDLcdrom = getenv("SDL_CDROM");	/* ':' separated list of devices */
	if ( SDLcdrom != NULL ) {
		char *cdpath, *delim;
		cdpath = malloc(strlen(SDLcdrom)+1);
		if ( cdpath != NULL ) {
			strcpy(cdpath, SDLcdrom);
			SDLcdrom = cdpath;
			do {
				delim = strchr(SDLcdrom, ':');
				if ( delim ) {
					*delim++ = '\0';
				}
#ifdef DEBUG_CDROM
  fprintf(stderr, "Checking CD-ROM drive from SDL_CDROM: %s\n", SDLcdrom);
#endif
				if ( CheckDrive(SDLcdrom, NULL, &stbuf) > 0 ) {
					AddDrive(SDLcdrom, &stbuf);
				}
				if ( delim ) {
					SDLcdrom = delim;
				} else {
					SDLcdrom = NULL;
				}
			} while ( SDLcdrom );
			free(cdpath);
		}

		/* If we found our drives, there's nothing left to do */
		if ( SDL_numcds > 0 ) {
			return(0);
		}
	}

	/* Scan the system for CD-ROM drives */
	for ( i=0; checklist[i]; ++i ) {
		if ( checklist[i][0] == '?' ) {
			char *insert;
			exists = 1;
			for ( j=checklist[i][1]; exists; ++j ) {
				sprintf(drive, "/dev/%s", &checklist[i][3]);
				insert = strchr(drive, '?');
				if ( insert != NULL ) {
					*insert = j;
				}
#ifdef DEBUG_CDROM
  fprintf(stderr, "Checking possible CD-ROM drive: %s\n", drive);
#endif
				switch (CheckDrive(drive, NULL, &stbuf)) {
					/* Drive exists and is a CD-ROM */
					case 1:
						AddDrive(drive, &stbuf);
						break;
					/* Drive exists, but isn't a CD-ROM */
					case 0:
						break;
					/* Drive doesn't exist */
					case -1:
						exists = 0;
						break;
				}
			}
		} else {
			sprintf(drive, "/dev/%s", checklist[i]);
#ifdef DEBUG_CDROM
  fprintf(stderr, "Checking possible CD-ROM drive: %s\n", drive);
#endif
			if ( CheckDrive(drive, NULL, &stbuf) > 0 ) {
				AddDrive(drive, &stbuf);
			}
		}
	}
	return(0);
}

static const char *SDL_SYS_CDName(int drive)
{
	return(SDL_cdlist[drive]);
}

static int SDL_SYS_CDOpen(int drive)
{
	return(open(SDL_cdlist[drive], (O_RDONLY|O_EXCL|O_NONBLOCK), 0));
}

static int SDL_SYS_CDGetTOC(SDL_CD *cdrom)
{
	cdrom_read_toc_t toc;
	int i, okay;

	okay = 0;
	if (devctl(cdrom->id, DCMD_CAM_CDROMREADTOC, &toc, sizeof(toc), 0) == 0) {
		cdrom->numtracks = toc.last_track - toc.first_track + 1;
		if ( cdrom->numtracks > SDL_MAX_TRACKS ) {
			cdrom->numtracks = SDL_MAX_TRACKS;
		}
		/* Read all the track TOC entries */
		for ( i=0; i<=cdrom->numtracks; ++i ) {
			if ( i == cdrom->numtracks ) {
				cdrom->track[i].id = CDROM_LEADOUT;
			} else {
#if 0 /* Sam 11/6/00 - an obsolete field? */
				cdrom->track[i].id = toc.cdth_trk0+i;
#else
				cdrom->track[i].id = toc.first_track+i;
#endif
			}
			cdrom->track[i].type = toc.toc_entry[i].control_adr;
			cdrom->track[i].offset = MSF_TO_FRAMES(
						toc.toc_entry[i].addr.msf.minute,
						toc.toc_entry[i].addr.msf.second,
						toc.toc_entry[i].addr.msf.frame);			
			cdrom->track[i].length = 0;
			if ( i > 0 ) {
				cdrom->track[i-1].length =
					cdrom->track[i].offset-
					cdrom->track[i-1].offset;
			}
		}
		if ( i == (cdrom->numtracks+1) ) {
			okay = 1;
		}
	}
	return(okay ? 0 : -1);
}

/* Get CD-ROM status */
static CDstatus SDL_SYS_CDStatus(SDL_CD *cdrom, int *position)
{
	CDstatus status;
	cdrom_read_toc_t toc;
	subch_current_position_t info;

#if 0 /* Sam 11/6/00 - an obsolete field? */
	info.subch_command.data_format = CDROM_SUBCH_CURRENT_POSITION;
	info.subch_command.track_number = 0;
#else
	info.data_format = CDROM_SUBCH_CURRENT_POSITION;
	info.track_number = 0;
#endif
	if (devctl(cdrom->id, DCMD_CAM_CDROMSUBCHNL, &info, sizeof(info), 0) != 0) {
		if ( ERRNO_TRAYEMPTY(errno) ) {
			status = CD_TRAYEMPTY;
		} else {
			status = CD_ERROR;
		}
	} else {
		switch (info.header.audio_status) {
			case CDROM_AUDIO_INVALID:
			case CDROM_AUDIO_NO_STATUS:
				/* Try to determine if there's a CD available */
				if (devctl(cdrom->id, DCMD_CAM_CDROMREADTOC, &toc, sizeof(toc), 0)==0)
					status = CD_STOPPED;
				else
					status = CD_TRAYEMPTY;
				break;
			case CDROM_AUDIO_COMPLETED:
				status = CD_STOPPED;
				break;
			case CDROM_AUDIO_PLAY:
				status = CD_PLAYING;
				break;
			case CDROM_AUDIO_PAUSED:
				/* Workaround buggy CD-ROM drive */
				if ( info.data_format == CDROM_LEADOUT ) {
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
					info.addr.msf.minute,
					info.addr.msf.second,
					info.addr.msf.frame);
		} else {
			*position = 0;
		}
	}
	return(status);
}

/* Start play */
static int SDL_SYS_CDPlay(SDL_CD *cdrom, int start, int length)
{
	cdrom_playmsf_t playtime;

	FRAMES_TO_MSF(start,
	   &playtime.start_minute, &playtime.start_second, &playtime.start_frame);
	FRAMES_TO_MSF(start+length,
	   &playtime.end_minute, &playtime.end_second, &playtime.end_frame);
#ifdef DEBUG_CDROM
  fprintf(stderr, "Trying to play from %d:%d:%d to %d:%d:%d\n",
	playtime.start_minute, playtime.start_second, playtime.start_frame,
	playtime.end_minute, playtime.end_second, playtime.end_frame);
#endif
	return(devctl(cdrom->id, DCMD_CAM_CDROMPLAYMSF, &playtime, sizeof(playtime), 0));
}

/* Pause play */
static int SDL_SYS_CDPause(SDL_CD *cdrom)
{
	return(devctl(cdrom->id, DCMD_CAM_CDROMPAUSE, NULL, 0, 0));
}

/* Resume play */
static int SDL_SYS_CDResume(SDL_CD *cdrom)
{
	return(devctl(cdrom->id, DCMD_CAM_CDROMRESUME, NULL, 0, 0));
}

/* Stop play */
static int SDL_SYS_CDStop(SDL_CD *cdrom)
{
	return(devctl(cdrom->id, DCMD_CAM_CDROMSTOP, NULL, 0, 0));
}

/* Eject the CD-ROM */
static int SDL_SYS_CDEject(SDL_CD *cdrom)
{
	return(devctl(cdrom->id, DCMD_CAM_EJECT_MEDIA, NULL, 0, 0));
}

/* Close the CD-ROM handle */
static void SDL_SYS_CDClose(SDL_CD *cdrom)
{
	close(cdrom->id);
}

void SDL_SYS_CDQuit(void)
{
	int i;

	if ( SDL_numcds > 0 ) {
		for ( i=0; i<SDL_numcds; ++i ) {
			free(SDL_cdlist[i]);
		}
		SDL_numcds = 0;
	}
}

