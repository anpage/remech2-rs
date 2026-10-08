#include "cmdline.h"

#include "bwd.h"
#include "clock.h"
#include "decomp.h"
#include "error.h"
#include "files.h"
#include "gamekeys.h"
#include "logwindow.h"
#include "mw2log.h"
#include "mw2prj.h"
#include "network.h"
#include "overlay.h"
#include "quadtree.h"
#include "render.h"
#include "resource.h"
#include "simmain.h"
#include "supanim.h"
#include "types.h"
#include "videodriverchoice.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Parses the command line MECH2.EXE passes: switches start with / or -, anything else names the
// mission (p_mission, "s.$" by default), which is shown with MonoPrintLine. /C sets p_flags[0];
// /S and /X clear p_flags[1]. Returns FALSE without a command line (not launched by MECH2.EXE).
// Stack-slot permutation of the locals.
// FUNCTION: MW2 0x1001ee80
MechS32 ProcessCmdLineArgs(MechChar* p_cmdLine, undefined4* p_flags, MechChar* p_mission)
{
	MechS32 result;
	MechChar* arg;
	MechChar* value;
	MechS32 rate;

	result = TRUE;
	if (!p_cmdLine) {
		printf("This program must be launched from MECH2.EXE\n");
		result = FALSE;
	}

	strcpy(p_mission, "s.$");
	arg = strtok(p_cmdLine, " ,");
	while (arg) {
		if (*arg == '/' || *arg == '-') {
			switch (toupper(arg[1])) {
			case 'B':
				value = strchr(arg, '=');
				if (value) {
					value++;
					g_supAnimBackdropName = value;
				}
				break;
			case 'C':
				p_flags[0] = 1;
				break;
			case 'D':
				g_missionTimerStopped = 1;
				break;
			case 'E':
				g_logStreams = 1;
				break;
			case 'F':
				rate = 10;
				if (arg[2] == '=') {
					rate = strtol(arg + 3, NULL, 10);
				}
				g_framerateLimit = 181 / rate;
				break;
			case 'G':
				value = strchr(arg, '=');
				if (value) {
					value++;
					g_supAnimShapeName = value;
				}
				break;
			case 'J':
				if (arg[2] == '=') {
					g_mw2PrjPath = arg + 3;
				}
			case 'M':
				InitializeMono();
				g_missionTimerStopped = 1;
				break;
			case 'N':
				g_isNetworkGame = 1;
				if (arg[2] == '=') {
					g_isNetworkGame = strtol(arg + 3, NULL, 10);
				}
				break;
			case 'O':
				if (arg[3] == '=') {
					switch (toupper(arg[2])) {
					case 'F':
						g_playerTeamFormation = arg + 4;
						break;
					case 'E':
						g_otherTeamFormation = arg + 4;
						break;
					}
				}
				break;
			case 'P':
				g_streamsFromFiles = 1;
				break;
			case 'Q':
				DisableQuadtrees();
				break;
			case 'R':
				g_startOnAutopilot = 1;
				break;
			case 'S':
				p_flags[1] = 0;
				break;
			case 'V':
				if (toupper(arg[2]) == 'G') {
					g_videoDriverChoice.m_flags |= 1;
					value = strchr(arg, '=');
					if (!value) {
						g_videoDriverChoice.m_name[0] = '\0';
					}
					else {
						value++;
						strncpy(g_videoDriverChoice.m_name, value, 12);
						g_videoDriverChoice.m_name[12] = '\0';
					}
				}
				else if (arg[2] == '=' && isdigit(arg[3])) {
					g_drawModeIndex = atoi(arg + 3);
				}
				break;
			case '1':
				g_netRole = 1;
				if (arg[2] == '=') {
					g_sessionName = arg + 3;
				}
				break;
			case '2':
				g_netRole = 2;
				if (arg[2] == '=') {
					g_sessionName = arg + 3;
				}
				break;
			default:
				Error(10, "%s", arg);
				break;
			}
		}
		else {
			strcpy(p_mission, arg);
		}

		arg = strtok(NULL, " ,");
	}

	MonoPrintLine(p_mission);
	return result;
}
