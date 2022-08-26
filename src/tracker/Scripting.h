/*
 *  tracker/ScopesControl.h
 *
 *  Copyright 2022 Coderofsalvation / Leon van Kammen 
 *
 *  This file is part of Milkytracker.
 *
 *  Milkytracker is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Milkytracker is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Milkytracker.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 *  Scripting.h
 *  A simple scripting-bridge for MilkyTracker SampleEditor
 *  Instead of bloating milkytracker with more effects/plugins, the burden of maintenance is reversed: external 
 *  applications & scripts can 'hook' into the sample-editor using the 'Script'-contextmenu.
 * 
 *  Created by coderofsalvation / Leon van Kammen on 26-10-2022 
 */

#ifndef SCRIPT_H
#define SCRIPT_H

#include "Tracker.h"
#include "ModuleEditor.h"
#include "PPUI.h"
#include "DialogBase.h"

/*
 * a list of scripting backends can be defined by 
 * the enduser (using the configfile below)
 */

#define SCRIPTS_MAX 75
#define SCRIPTS_TOKENS 3
#define SCRIPTS_FORMAT "%[^';'];%[^'|']|%[^'\n']\n"
//                      <name>;<cmd fmstring>|<exec|extension_for_filedialog>\n


// default example scripts (console will warn if dependencies are not installed)
// more scripts can be found here: https://gitlab.com/coderofsalvation/milkytracker-scripts
#if defined(WINDOWS) || defined(WIN32) // C++ >= v17
#define SCRIPTS_DEFAULT "\
editor/wavosaur;echo %s && Wavosaur.exe %s && echo %s|exec\n\
csound/run patch;csound.exe --nosound --strset1=%s --strset2=%s --strset3=%s %s|csd\n\
puredata/run patch;pd.exe -send \"in %s\" -send \"out %s\" -send \"clipboard %s\" %s|pd\n\
ffmpeg/phaser;ffmpeg.exe -y -i %s -af \"aphaser=out_gain=0.8:in_gain=0.8\" %s|exec\n\
sox/timestretch+;sox -D %s %s tempo 0.75 10 20 30|exec\n\
sox/timestretch-;sox -D %s %s tempo 1.25 10 20 30|exec\n\
sox/reverb+;sox.exe -D %s %s pad 0 1.5 reverb 50 50 100 0 50|exec\n\
sox/reverb++;sox.exe -D %s %s pad 0 4 reverb 100 50 100 0 50|exec\n\
sox/reverb wet+;sox.exe -D %s %s pad 0 1.5 reverb -w 50 50 100 0 50|exec\n\
sox/reverb wet++;sox.exe -D %s %s pad 0 4 reverb -w 100 50 100 0 50|exec\n\
sox/synth note;echo %s && sox.exe -n -c1 -r44100 -b 16 %s synth 0.5 sine 200-500 synth 0.5 sine fmod 700-100|exec\n"
#else
#define SCRIPTS_DEFAULT "\
editor/mhwaveedit;echo %s && mhwaveedit %s|exec\n\
csound/run patch;csound --nosound --strset1=%s --strset2=%s --strset3=%s %s|csd\n\
puredata/run patch;pd -send \"in %s\" -send \"out %s\" -send \"clipboard %s\" %s|pd\n\
ffmpeg/phaser;ffmpeg -y -i %s -af \"aphaser=out_gain=0.8:in_gain=0.8\" %s|exec\n\
sox/timestretch+;sox -D %s %s tempo 0.75 10 20 30|exec\n\
sox/timestretch-;sox -D %s %s tempo 1.25 10 20 30|exec\n\
sox/reverb+;sox -D %s %s pad 0 1.5 reverb 50 50 100 0 50|exec\n\
sox/reverb++;sox -D %s %s pad 0 4 reverb 100 50 100 0 50|exec\n\
sox/reverb wet+;sox -D %s %s pad 0 1.5 reverb -w 50 50 100 0 50|exec\n\
sox/reverb wet++;sox -D %s %s pad 0 4 reverb -w 100 50 100 0 50|exec\n\
sox/synth note;echo %s && sox -n -c1 -r44100 -b 16 %s synth 0.5 sine 200-500 synth 0.5 sine fmod 700-100|exec\n"
#endif

class Scripting{
	public:

		static const int MenuID = 10000;
		static const int MenuIDHelp = MenuID+SCRIPTS_MAX;
		static FILE *scripts;

		static void loadScripts(PPString file){
			file.append(".scripts.txt");
			scripts = fopen( file.getStrBuffer(), "r");
			if( scripts == NULL ){ // create default
				scripts = fopen( file.getStrBuffer(), "w");
				fprintf(scripts,"%s",SCRIPTS_DEFAULT);
				fclose(scripts);
				scripts = fopen( file.getStrBuffer(), "r");
			}
		}

		static void loadScriptsToMenu(PPString scriptsPath, PPContextMenu *m ){
			// # <name>;<binary>;<cmd>;<exec|extension_for_filedialog>\n
			char name[100], cmd[255], ext[20];
			int  i = 0;
			loadScripts(scriptsPath);
			while( fscanf(scripts, SCRIPTS_FORMAT, name, cmd, ext) == SCRIPTS_TOKENS && i < SCRIPTS_MAX ){
				m->addEntry( name, MenuID+i);
				i++;
			}
			fclose(scripts);
		}

		static int runScript(PPString configPath, int ID, char *cmd, PPScreen *screen, PPString fin, PPString fout, PPString *selected ){
			int  i = 0;
			char name[100], ext[20];
			loadScripts(configPath);
			while( fscanf(scripts, SCRIPTS_FORMAT, name, cmd, ext) == SCRIPTS_TOKENS && i < SCRIPTS_MAX ){
				if( MenuID+i == ID ){ 
					PPString file;
					PPString fclipboard;
					char finalcmd[255];
					printf("running %s %s\n",ext,"exec");
					*selected = PPString(name).subString(0,24);
					if( strncmp("exec",ext,4) != 0 ) filepicker(name,ext,&file,screen);
					sprintf(finalcmd, cmd, fin.getStrBuffer(),
																 fout.getStrBuffer(),
																 fclipboard.getStrBuffer(),
																 file.getStrBuffer() );
					printf("> %s\n",finalcmd);
					return system(finalcmd);
				}
				i++;
			}
			return -1;
		}

		static void filepicker(char *name, char *ext, PPString *result, PPScreen *screen ){
			PPOpenPanel* openPanel = NULL;
			const char *extensions[] = {
				ext,name,
				NULL,NULL
			};
			openPanel = new PPOpenPanel(screen, name );
			openPanel->addExtensions(extensions);
			if (!openPanel) return;
			bool res = true;
			if (openPanel->runModal() == PPModalDialog::ReturnCodeOK)
			{
				*result = openPanel->getFileName();
				delete openPanel;
			}
		}
};

FILE* Scripting::scripts              = NULL;

#endif

