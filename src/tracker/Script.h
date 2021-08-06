#ifndef SAMPLEDITOR_SCRIPT_H
#define SAMPLEDITOR_SCRIPT_H

/* 
 * Script - THE MILKY GATEWAY
 * --------------------------
 * The milky gateway basically is 2 things: milkymacro's (c++ batchprocessor) OR external scripts..all thru the power of unix SHEBANG (https://en.wikipedia.org/wiki/Shebang_(Unix)).
 * Therefore, its a deadsimple GATEWAY, rather than embedding a full scripting language (=out of scope).
 * Choosing a trendy language X (Chaiscript/Lua/micropython/Javascript e.g.) is easy, but it would cause 
 * a forever-lock-in with future compatibility/maintenance/userscripts.
 * To avoid this rabbithole: we'll do only batch (milkymacro) OR forward wav-files to our own preferred scripting languages / cli tools (shebang).
 * 
 * The advice above has been given to humanity thru enlightened souls.
 * Please tell users to use their favorite language using the shebang, whenever they suggest extending MilkyMacro's with variables, 'if/else logic' or 'language feature X'.
 * May the force be with you. (author: Leon van Kammen)
 */

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <fstream>
#include <streambuf>
#include <map>
#include "FilterParameters.h"
#include "ControlIDs.h"
#include "ModuleEditor.h"
#include "Tracker.h"
#if defined("DRIVER_UNIX")
#include <unistd.h>
#endif
      
#define BUFSIZE 255

using namespace std;

class MilkyMacro {

public:
	string err;
	Tracker *tracker;
	
	bool MACRO(const char* funcall, float* v, string line) {
		string fc = string(funcall);
		int nvars = 0;
		for (int i = 0; i < fc.size(); i++)
			if (fc[i] == '%') nvars++;
		return sscanf(line.c_str(), funcall, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5]) == nvars ? true : false;
	}

	bool run(PPString file) {
		err = "";
		string line;
		// read file
		std::ifstream t(file);
		std::string script((std::istreambuf_iterator<char>(t)),std::istreambuf_iterator<char>());
		t.close();
		// parse file
		istringstream s( string(script) + string("\n") );
		while (getline(s, line) && !s.eof() ) {
			if (line == string("exit")) break;
			if (line[0] == '#' || line[0] == '\n' || line.length() == 0     ) continue; 
			if (tracker->runMacro((MilkyMacro*)this, PPString(line.c_str()))) continue;
			else {
				err = string("error: ") + line;
				break;
			}
		}
		return err.length() == 0;
	}

};

class Script{
  private:
    FILE *f;
	char cdir[BUFSIZE];

  public:
	Tracker* tracker;
	ModuleEditor* moduleEditor;
	MilkyMacro macro;
	bool outputFile;
	string err;
	
  int exec( PPString file, PPString fin, PPString fout ){
    outputFile = false;
    err = "";
    char s[BUFSIZ]={ 0 };
    int l=0;
    f=fopen(file.getStrBuffer(),"r");
    if (f==0) {
      tracker->showMessageBoxSized(MESSAGEBOX_UNIVERSAL, "could not open script", Tracker::MessageBox_OK);
      return 0; // because 0 will 
    }
    fgets(s,BUFSIZ,f);
    while ( l < BUFSIZ ) {
      if (s[l]=='\r'||s[l]=='\n'||s[l]==EOF) {
        s[l]='\0';
        break;
      }
      l++;
    }
    fclose(f);

    char launcher[BUFSIZ]={ 0 };
    char cmd[BUFSIZ]={ 0 };
    int ok = 0;
    if(s[0]=='#' && s[1]=='!') {

      /* 
       * WINDOWS: automatically pops up terminal for interactive scripts
       * LINUX:   pops up X11 terminal or launches script as non-interactive
       * AMIGA:   *TODO* launch 'auto con' console window https://wiki.amigaos.net/wiki/Executing_External_Programs
       */
      #ifdef defined(DRIVER_UNIX)
      if( access("/usr/bin/x-terminal-emulator" , F_OK ) == 0 ) {
        snprintf(launcher,BUFSIZ,"/usr/bin/x-terminal-emulator %s",s+2);
      }else snprintf(launcher,BUFSIZ,"%s",s+2);
      #else
      snprintf(launcher,BUFSIZ,"%s",s+2);
      #endif

      /////////////////////////////////////////////////////////////////////////////////// MILKY MACRO
      if (strstr(launcher, "milkymacro v") != NULL) {
        outputFile = false;
        ok = macro.run(file) ? 0 : 1;
        if (macro.err.length() > 0) err = macro.err; 
      }else {
        ///////////////////////////////////////////////////////////////////////////////// FORMATSTRING SHEBANG
        if (strstr(launcher, "%s") != NULL) {
          snprintf(cmd, BUFSIZ, launcher, fin.getStrBuffer(), fout.getStrBuffer());
        }
        // TRADITIONAL SHEBANG
        else {
          snprintf(cmd, BUFSIZ, "%s \"%s\" \"%s\" \"%s\"", launcher, file.getStrBuffer(), fin.getStrBuffer(), fout.getStrBuffer());
        }
      }
      // RUN! +remove last produced out.wav file since certain utilities don't like leftovers
      remove(fout.getStrBuffer());
      ok = system(cmd);	
      outputFile = ok == 0;
      // notify errors
      if (ok != 0 || err.length() > 0) {
        char num[50];
        sprintf(num, "%i", ok);
        err = err.length() > 0 ? err : string("exitcode: ") + string(num);
        tracker->showMessageBoxSized(MESSAGEBOX_UNIVERSAL, err.c_str(), Tracker::MessageBox_OK);
      }
      return ok;
    } else {
      tracker->showMessageBoxSized(MESSAGEBOX_UNIVERSAL, "script is missing shebang", Tracker::MessageBox_OK);
      return 1;
    }
  }

	int init(PPString fin, PPString fout) {
		// create new in.wav
		int selected_instrument;
		int selected_sample;
		tracker->getSelectedInstrument(&selected_instrument, &selected_sample);
		int res = moduleEditor->saveSample(
			fin, 
			selected_instrument, 
			selected_sample,
			ModuleEditor::SampleFormatTypeWAV
		);
		return res;
	}

	void update(PPString fin, PPString fout) {
		int selected_instrument;
		int selected_sample;
		
		// return if fout does not exist 
		FILE* ffout = fopen(fout.getStrBuffer(), "r");
		if ( ffout  == NULL) return;
		fclose(ffout);

		tracker->getSelectedInstrument(&selected_instrument, &selected_sample);
		moduleEditor->loadSample(
			fout,
			selected_instrument,
			selected_sample,
			ModuleEditor::SampleFormatTypeWAV
		);
	}

};



#endif
